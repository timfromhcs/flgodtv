#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <variant>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "flgod/core/entity_id.hpp"
#include "flgod/agents/agent_types.hpp"
#include "flgod/agents/agent_manager.hpp"

namespace flgod::llm {

enum class SecurityStage {
    None,
    Parse,
    SchemaValidation,
    CapabilityValidation,
    WorldValidation,
    Execution
};

inline const char* security_stage_to_string(SecurityStage stage) {
    switch (stage) {
        case SecurityStage::None: return "None";
        case SecurityStage::Parse: return "Parse";
        case SecurityStage::SchemaValidation: return "SchemaValidation";
        case SecurityStage::CapabilityValidation: return "CapabilityValidation";
        case SecurityStage::WorldValidation: return "WorldValidation";
        case SecurityStage::Execution: return "Execution";
    }
    return "Unknown";
}

enum class ValidatedActionType {
    Speech,
    Demonstration,
    Concept,
    Suggestion,
    Explanation,
    Move,
    Forage,
    Idle,
    Signal
};

struct ValidatedLLMAction {
    ValidatedActionType type{ValidatedActionType::Idle};
    EntityID sender_id{0};
    EntityID target_agent_id{0};
    std::string text_content;
    std::string concept_id;
    Vec3 target_position{0.0, 0.0, 0.0};
    int signal_id{0};
    bool is_valid{false};
};

struct SecurityValidationResult {
    bool passed{false};
    SecurityStage failed_stage{SecurityStage::None};
    std::string error_reason;
    ValidatedLLMAction action;
};

class ActionSecurityValidator {
private:
    std::unordered_set<std::string> m_forbidden_keys = {
        "exec", "system", "command", "cmd", "shell", "eval", "file", "path", 
        "script", "download", "upload", "url", "http", "https", "socket", "env"
    };

    static std::string strip_code_fences(const std::string& input) {
        std::string s = input;
        size_t start = s.find("```");
        if (start != std::string::npos) {
            size_t nl = s.find('\n', start);
            if (nl != std::string::npos) {
                s = s.substr(nl + 1);
            } else {
                s = s.substr(start + 3);
            }
            size_t end = s.rfind("```");
            if (end != std::string::npos) {
                s = s.substr(0, end);
            }
        }
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, (last - first + 1));
    }

public:
    ActionSecurityValidator() = default;

    SecurityValidationResult validate_and_parse(
        const std::string& raw_llm_output,
        EntityID sender_id,
        bool is_god_fly_teacher,
        const AgentManager* agent_manager = nullptr,
        double max_comm_range = 100.0) 
    {
        SecurityValidationResult result;
        result.passed = false;
        result.action.sender_id = sender_id;

        // Stage 1: Parse
        std::string clean_json = strip_code_fences(raw_llm_output);
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(clean_json);
        } catch (const std::exception& e) {
            result.failed_stage = SecurityStage::Parse;
            result.error_reason = std::string("JSON parse error: ") + e.what();
            return result;
        }

        if (!j.is_object()) {
            result.failed_stage = SecurityStage::Parse;
            result.error_reason = "Root JSON element must be an object";
            return result;
        }

        // Stage 2: Schema Validation
        if (!j.contains("action") || !j["action"].is_string()) {
            result.failed_stage = SecurityStage::SchemaValidation;
            result.error_reason = "Missing or invalid 'action' field";
            return result;
        }

        std::string action_str = j["action"].get<std::string>();

        // Check for forbidden keywords in entire JSON structure
        auto check_forbidden = [&](const nlohmann::json& elem, auto& self) -> bool {
            if (elem.is_object()) {
                for (auto it = elem.begin(); it != elem.end(); ++it) {
                    std::string key_lower = it.key();
                    std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
                    if (m_forbidden_keys.count(key_lower) > 0) return true;
                    if (self(it.value(), self)) return true;
                }
            } else if (elem.is_array()) {
                for (const auto& item : elem) {
                    if (self(item, self)) return true;
                }
            } else if (elem.is_string()) {
                std::string s_lower = elem.get<std::string>();
                std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(), ::tolower);
                for (const auto& fk : m_forbidden_keys) {
                    if (s_lower.find(fk) != std::string::npos && (fk == "exec" || fk == "shell" || fk == "system")) {
                        return true;
                    }
                }
            }
            return false;
        };

        if (check_forbidden(j, check_forbidden)) {
            result.failed_stage = SecurityStage::SchemaValidation;
            result.error_reason = "Forbidden dangerous keyword or sandbox escape detected in JSON payload";
            return result;
        }

        // Extract parameters safely
        nlohmann::json params = nlohmann::json::object();
        if (j.contains("parameters") && j["parameters"].is_object()) {
            params = j["parameters"];
        }

        ValidatedActionType act_type;
        if (action_str == "speech") act_type = ValidatedActionType::Speech;
        else if (action_str == "demonstration") act_type = ValidatedActionType::Demonstration;
        else if (action_str == "concept") act_type = ValidatedActionType::Concept;
        else if (action_str == "suggestion") act_type = ValidatedActionType::Suggestion;
        else if (action_str == "explanation") act_type = ValidatedActionType::Explanation;
        else if (action_str == "move") act_type = ValidatedActionType::Move;
        else if (action_str == "forage") act_type = ValidatedActionType::Forage;
        else if (action_str == "idle") act_type = ValidatedActionType::Idle;
        else if (action_str == "signal") act_type = ValidatedActionType::Signal;
        else {
            result.failed_stage = SecurityStage::SchemaValidation;
            result.error_reason = "Unrecognized or disallowed action: " + action_str;
            return result;
        }

        result.action.type = act_type;

        // Stage 3: Capability Validation
        bool is_teacher_action = (act_type == ValidatedActionType::Concept ||
                                  act_type == ValidatedActionType::Demonstration ||
                                  act_type == ValidatedActionType::Explanation ||
                                  act_type == ValidatedActionType::Suggestion);

        if (is_teacher_action && !is_god_fly_teacher) {
            result.failed_stage = SecurityStage::CapabilityValidation;
            result.error_reason = "Agent lacks teacher capability for action: " + action_str;
            return result;
        }

        // Stage 4: World Validation
        if (params.contains("content") && params["content"].is_string()) {
            result.action.text_content = params["content"].get<std::string>();
            if (result.action.text_content.size() > 512) {
                result.action.text_content = result.action.text_content.substr(0, 512);
            }
        }

        if (params.contains("concept_id") && params["concept_id"].is_string()) {
            result.action.concept_id = params["concept_id"].get<std::string>();
        }

        if (params.contains("target_agent")) {
            if (params["target_agent"].is_number_unsigned() || params["target_agent"].is_number_integer()) {
                result.action.target_agent_id = EntityID(params["target_agent"].get<uint64_t>());
            }
        }

        if (params.contains("position") && params["position"].is_array() && params["position"].size() == 3) {
            result.action.target_position = Vec3{
                params["position"][0].get<double>(),
                params["position"][1].get<double>(),
                params["position"][2].get<double>()
            };
        }

        if (agent_manager != nullptr) {
            bool sender_exists = agent_manager->has_agent(sender_id);
            if (!sender_exists && !is_god_fly_teacher) {
                result.failed_stage = SecurityStage::WorldValidation;
                result.error_reason = "Sender agent does not exist in world";
                return result;
            }

            if (result.action.target_agent_id.raw() != 0) {
                if (!agent_manager->has_agent(result.action.target_agent_id)) {
                    result.failed_stage = SecurityStage::WorldValidation;
                    result.error_reason = "Target agent " + std::to_string(result.action.target_agent_id.raw()) + " does not exist in world";
                    return result;
                }

                const auto& target = agent_manager->get_agent(result.action.target_agent_id);
                if (!target.is_alive()) {
                    result.failed_stage = SecurityStage::WorldValidation;
                    result.error_reason = "Target agent is dead";
                    return result;
                }

                if (sender_exists) {
                    const auto& sender = agent_manager->get_agent(sender_id);
                    double dist = (sender.position() - target.position()).length();
                    if (dist > max_comm_range) {
                        result.failed_stage = SecurityStage::WorldValidation;
                        result.error_reason = "Target agent out of communication range (" + std::to_string(dist) + " > " + std::to_string(max_comm_range) + ")";
                        return result;
                    }
                }
            }
        }

        // Stage 5: Ready for Execution
        result.passed = true;
        result.action.is_valid = true;
        return result;
    }
};

} // namespace flgod::llm
