#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "flgod/core/entity_id.hpp"
#include "flgod/agents/agent.hpp"
#include "flgod/learning/memory.hpp"
#include "flgod/llm/model_manager.hpp"
#include "flgod/llm/action_security.hpp"

namespace flgod::llm {

enum class TeachingMode {
    Speech,
    Demonstration,
    Concept,
    Suggestion,
    Explanation
};

inline const char* teaching_mode_to_string(TeachingMode mode) {
    switch (mode) {
        case TeachingMode::Speech: return "Speech";
        case TeachingMode::Demonstration: return "Demonstration";
        case TeachingMode::Concept: return "Concept";
        case TeachingMode::Suggestion: return "Suggestion";
        case TeachingMode::Explanation: return "Explanation";
    }
    return "Unknown";
}

struct TeachingPacket {
    TeachingMode mode{TeachingMode::Speech};
    std::string content;
    std::string concept_id;
    AgentActionType suggested_action{AgentActionType::Idle};
    std::vector<Vec3> demonstration_path;
    EntityID student_id{0};
    uint64_t timestamp{0};
    bool delivered{false};

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["mode"] = teaching_mode_to_string(mode);
        j["content"] = content;
        j["concept_id"] = concept_id;
        j["student_id"] = student_id.raw();
        j["timestamp"] = timestamp;
        return j;
    }
};

class GodFly {
private:
    EntityID m_id{1000000000000ULL}; // High unique ID for God Fly entity
    MemorySystem m_memory;
    ModelManager* m_model_manager{nullptr};
    ActionSecurityValidator m_security_validator;
    uint64_t m_total_lessons_taught{0};
    Vec3 m_position{30.0, 8.0, 30.0};

public:
    explicit GodFly(ModelManager* mgr = nullptr) : m_model_manager(mgr) {}

    [[nodiscard]] EntityID get_id() const noexcept { return m_id; }
    [[nodiscard]] const MemorySystem& get_memory() const noexcept { return m_memory; }
    [[nodiscard]] MemorySystem& get_memory() noexcept { return m_memory; }
    [[nodiscard]] uint64_t get_total_lessons_taught() const noexcept { return m_total_lessons_taught; }
    [[nodiscard]] const Vec3& position() const noexcept { return m_position; }
    void set_position(const Vec3& pos) noexcept { m_position = pos; }

    void set_model_manager(ModelManager* mgr) noexcept { m_model_manager = mgr; }

    // Generates a lesson via the local model and verifies it through the 5-stage security pipeline
    bool create_lesson(TeachingMode mode,
                       EntityID target_student_id,
                       const std::string& concept_id,
                       const std::string& context_info,
                       TeachingPacket& out_packet,
                       const AgentManager* agent_mgr = nullptr) 
    {
        if (!m_model_manager || !m_model_manager->is_loaded(ModelRole::GodFly)) {
            return false;
        }

        std::string mode_str = "concept";
        AgentActionType suggested_act = AgentActionType::Forage;
        if (mode == TeachingMode::Speech) mode_str = "speech";
        else if (mode == TeachingMode::Demonstration) { mode_str = "demonstration"; suggested_act = AgentActionType::Move; }
        else if (mode == TeachingMode::Suggestion) { mode_str = "suggestion"; suggested_act = AgentActionType::Forage; }
        else if (mode == TeachingMode::Explanation) { mode_str = "explanation"; }

        std::string prompt = "Context: Student ID " + std::to_string(target_student_id.raw()) + 
                             " needs guidance on " + concept_id + ". Additional context: " + context_info;

        std::string simulated_response = "{\n"
            "  \"action\": \"" + mode_str + "\",\n"
            "  \"parameters\": {\n"
            "    \"concept_id\": \"" + concept_id + "\",\n"
            "    \"content\": \"Guidance: prioritize foraging at nearby nectar bloom to restore vital energy.\",\n"
            "    \"target_agent\": " + std::to_string(target_student_id.raw()) + "\n"
            "  }\n"
            "}";

        std::string raw_output = m_model_manager->generate_response(ModelRole::GodFly, prompt, simulated_response);

        auto sec_res = m_security_validator.validate_and_parse(
            raw_output, 
            m_id, 
            /*is_god_fly_teacher=*/true, 
            agent_mgr
        );

        if (!sec_res.passed) {
            return false;
        }

        out_packet.mode = mode;
        out_packet.content = sec_res.action.text_content;
        out_packet.concept_id = sec_res.action.concept_id;
        out_packet.suggested_action = suggested_act;
        out_packet.student_id = sec_res.action.target_agent_id;
        out_packet.timestamp = 1;
        out_packet.delivered = false;

        return true;
    }

    // Deliver teaching packet to student agent.
    // SECTION 54 STRICT COMPLIANCE:
    // "Learners must still experience: observation, practice, feedback, memory.
    // Never unlock skills magically because the teacher mentioned them."
    bool deliver_lesson(TeachingPacket& packet, Agent& student, double current_time) {
        if (student.id() != packet.student_id) {
            return false;
        }

        // 1. Student receives social observation of teacher's instruction into Working and Social Memory
        // NO FREE REWARD!
        student.memory().working().add_item(
            "lesson_" + packet.concept_id, 
            {static_cast<double>(packet.suggested_action), 0.0, 0.0}
        );
        student.memory().social().update_interaction(m_id.raw(), /*valence=*/0.5, current_time);

        // 2. Student adds concept hypothesis to Semantic Memory with low initial confidence (0.25)
        // Full confidence must be earned through actual physical practice!
        student.memory().semantic().store_fact(packet.concept_id, {static_cast<double>(packet.suggested_action)}, 0.25);

        packet.delivered = true;
        m_total_lessons_taught++;
        return true;
    }
};

} // namespace flgod::llm
