#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "flgod/core/entity_id.hpp"
#include "flgod/agents/agent.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/llm/model_manager.hpp"
#include "flgod/llm/action_security.hpp"

namespace flgod::llm {

struct NPCPersonality {
    float curiosity{0.7f};
    float boldness{0.5f};
    float sociability{0.6f};
    float patience{0.5f};

    nlohmann::json to_json() const {
        return nlohmann::json{
            {"curiosity", curiosity},
            {"boldness", boldness},
            {"sociability", sociability},
            {"patience", patience}
        };
    }
};

struct NPCGoal {
    std::string name;
    float priority{1.0f};
    Vec3 target_location{0.0, 0.0, 0.0};
    bool is_completed{false};
};

struct DialogueTurn {
    EntityID speaker_id{0};
    std::string text;
    uint64_t timestamp{0};
};

enum class NPCInferenceTrigger {
    None,
    DirectDialogue,
    CriticalResource,
    TerritoryInvasion,
    NovelDiscovery
};

inline const char* trigger_to_string(NPCInferenceTrigger trigger) {
    switch (trigger) {
        case NPCInferenceTrigger::None: return "None";
        case NPCInferenceTrigger::DirectDialogue: return "DirectDialogue";
        case NPCInferenceTrigger::CriticalResource: return "CriticalResource";
        case NPCInferenceTrigger::TerritoryInvasion: return "TerritoryInvasion";
        case NPCInferenceTrigger::NovelDiscovery: return "NovelDiscovery";
    }
    return "Unknown";
}

class NPCAgent {
private:
    EntityID m_agent_id{0};
    NPCPersonality m_personality;
    std::vector<NPCGoal> m_goals;
    std::unordered_map<uint64_t, float> m_relationships;
    std::unordered_map<std::string, float> m_knowledge;
    std::vector<DialogueTurn> m_conversation_history;
    
    double m_last_inference_time{ -100.0 };
    double m_min_inference_cooldown_sec{ 3.0 };
    uint64_t m_inference_count{0};

public:
    explicit NPCAgent(EntityID id) : m_agent_id(id) {}

    [[nodiscard]] EntityID get_id() const noexcept { return m_agent_id; }
    [[nodiscard]] const NPCPersonality& get_personality() const noexcept { return m_personality; }
    NPCPersonality& get_personality() noexcept { return m_personality; }
    [[nodiscard]] const std::vector<NPCGoal>& get_goals() const noexcept { return m_goals; }
    [[nodiscard]] const std::vector<DialogueTurn>& get_history() const noexcept { return m_conversation_history; }
    [[nodiscard]] uint64_t get_inference_count() const noexcept { return m_inference_count; }

    void add_goal(const std::string& name, float priority, const Vec3& loc = {0.0, 0.0, 0.0}) {
        m_goals.push_back({name, priority, loc, false});
    }

    void set_relationship(EntityID other_id, float trust) {
        m_relationships[other_id.raw()] = std::clamp(trust, -1.0f, 1.0f);
    }

    [[nodiscard]] float get_trust(EntityID other_id) const noexcept {
        auto it = m_relationships.find(other_id.raw());
        if (it != m_relationships.end()) return it->second;
        return 0.0f;
    }

    void set_knowledge(const std::string& concept_name, float level) {
        m_knowledge[concept_name] = std::clamp(level, 0.0f, 1.0f);
    }

    [[nodiscard]] float get_knowledge(const std::string& concept_name) const noexcept {
        auto it = m_knowledge.find(concept_name);
        if (it != m_knowledge.end()) return it->second;
        return 0.0f;
    }

    void record_turn(EntityID speaker, const std::string& text, uint64_t timestamp) {
        m_conversation_history.push_back({speaker, text, timestamp});
        if (m_conversation_history.size() > 20) {
            m_conversation_history.erase(m_conversation_history.begin());
        }
    }

    // SECTION 55: "LLM inference is event-driven. Do not run an LLM continuously for every background agent."
    [[nodiscard]] bool should_trigger_inference(NPCInferenceTrigger trigger, double current_time) const noexcept {
        if (trigger == NPCInferenceTrigger::None) {
            return false;
        }
        if ((current_time - m_last_inference_time) < m_min_inference_cooldown_sec) {
            return false;
        }
        return true;
    }

    void mark_inference_executed(double current_time) noexcept {
        m_last_inference_time = current_time;
        m_inference_count++;
    }
};

class NPCManager {
private:
    std::unordered_map<uint64_t, NPCAgent> m_npcs;
    ModelManager* m_model_manager{nullptr};
    ActionSecurityValidator m_security_validator;

public:
    explicit NPCManager(ModelManager* mgr = nullptr) : m_model_manager(mgr) {}

    void set_model_manager(ModelManager* mgr) noexcept { m_model_manager = mgr; }

    NPCAgent* register_npc(EntityID id) {
        auto res = m_npcs.emplace(id.raw(), NPCAgent(id));
        return &(res.first->second);
    }

    NPCAgent* get_npc(EntityID id) {
        auto it = m_npcs.find(id.raw());
        if (it != m_npcs.end()) return &it->second;
        return nullptr;
    }

    const NPCAgent* get_npc(EntityID id) const {
        auto it = m_npcs.find(id.raw());
        if (it != m_npcs.end()) return &it->second;
        return nullptr;
    }

    [[nodiscard]] size_t count() const noexcept { return m_npcs.size(); }

    bool trigger_event_inference(EntityID npc_id, 
                                 NPCInferenceTrigger trigger, 
                                 double current_time,
                                 const std::string& event_context,
                                 ValidatedLLMAction& out_action,
                                 const AgentManager* agent_mgr = nullptr) 
    {
        NPCAgent* npc = get_npc(npc_id);
        if (!npc) return false;

        if (!npc->should_trigger_inference(trigger, current_time)) {
            return false;
        }

        if (!m_model_manager || !m_model_manager->is_loaded(ModelRole::NPC)) {
            return false;
        }

        std::string prompt = "NPC " + std::to_string(npc_id.raw()) + 
                             " Event: " + trigger_to_string(trigger) + 
                             ". Context: " + event_context;

        std::string simulated_response = "{\n"
            "  \"action\": \"speech\",\n"
            "  \"parameters\": {\n"
            "    \"content\": \"Noticed event: " + std::string(trigger_to_string(trigger)) + ", coordinating action.\",\n"
            "    \"target_agent\": 0\n"
            "  }\n"
            "}";

        std::string raw_output = m_model_manager->generate_response(ModelRole::NPC, prompt, simulated_response);

        auto sec_res = m_security_validator.validate_and_parse(
            raw_output,
            npc_id,
            /*is_god_fly_teacher=*/false,
            agent_mgr
        );

        if (!sec_res.passed) {
            return false;
        }

        npc->mark_inference_executed(current_time);
        npc->record_turn(npc_id, sec_res.action.text_content, static_cast<uint64_t>(current_time * 60.0));

        out_action = sec_res.action;
        return true;
    }
};

} // namespace flgod::llm
