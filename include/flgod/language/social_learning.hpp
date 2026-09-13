#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "flgod/core/entity_id.hpp"
#include "flgod/agents/agent_types.hpp"
#include "flgod/agents/agent.hpp"
#include "flgod/language/vocabulary.hpp"

namespace flgod::language {

// SECTION 57: "Track whether knowledge originated from: individual experience, social transfer, God Fly, cultural inheritance"
enum class KnowledgeOrigin : uint8_t {
    IndividualExperience = 0, // Discovered via autonomous exploration & trial-and-error
    SocialTransfer = 1,       // Transferred peer-to-peer within the same generation
    GodFly = 2,               // Taught directly by the God Fly teacher
    CulturalInheritance = 3   // Inherited across generations through colony cultural tradition
};

inline const char* knowledge_origin_to_string(KnowledgeOrigin origin) {
    switch (origin) {
        case KnowledgeOrigin::IndividualExperience: return "IndividualExperience";
        case KnowledgeOrigin::SocialTransfer: return "SocialTransfer";
        case KnowledgeOrigin::GodFly: return "GodFly";
        case KnowledgeOrigin::CulturalInheritance: return "CulturalInheritance";
    }
    return "Unknown";
}

// SECTION 57: "Implement: observation, imitation, demonstration, teaching, communication"
enum class TransmissionChannel : uint8_t {
    Observation = 0,    // Passive visual/sensory monitoring of peer actions
    Imitation = 1,      // Active replication of observed behavior
    Demonstration = 2,  // Intentional behavioral display with spatial path
    Teaching = 3,       // Structured pedagogical transmission (e.g. God Fly)
    Communication = 4   // Symbolic / acoustic / dance language exchange
};

inline const char* transmission_channel_to_string(TransmissionChannel ch) {
    switch (ch) {
        case TransmissionChannel::Observation: return "Observation";
        case TransmissionChannel::Imitation: return "Imitation";
        case TransmissionChannel::Demonstration: return "Demonstration";
        case TransmissionChannel::Teaching: return "Teaching";
        case TransmissionChannel::Communication: return "Communication";
    }
    return "Unknown";
}

struct KnowledgeProvenance {
    std::string concept_key;
    AgentActionType action{AgentActionType::Idle};
    KnowledgeOrigin origin{KnowledgeOrigin::IndividualExperience};
    TransmissionChannel channel{TransmissionChannel::Observation};
    EntityID source_agent_id{0};
    uint32_t generation_depth{0};            // 0 = primary origin, 1+ = social hops
    double transmission_fidelity{1.0};       // Degradation / noise over transmission hops
    double empirical_validation_score{0.0};  // Self-tested success score
    double acquisition_time{0.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"concept", concept_key},
            {"action", static_cast<int>(action)},
            {"origin", knowledge_origin_to_string(origin)},
            {"origin_id", static_cast<int>(origin)},
            {"channel", transmission_channel_to_string(channel)},
            {"channel_id", static_cast<int>(channel)},
            {"source_id", source_agent_id.raw()},
            {"gen_depth", generation_depth},
            {"fidelity", transmission_fidelity},
            {"empirical_score", empirical_validation_score},
            {"time", acquisition_time}
        };
    }
};

class SocialLearningTracker {
private:
    // Agent ID (raw) -> (Concept key -> KnowledgeProvenance)
    std::unordered_map<uint64_t, std::unordered_map<std::string, KnowledgeProvenance>> m_agent_knowledge;
    uint64_t m_transmission_events{0};

public:
    SocialLearningTracker() = default;

    void record_individual_discovery(EntityID agent_id, const std::string& concept_key, AgentActionType action, double time) {
        KnowledgeProvenance prov;
        prov.concept_key = concept_key;
        prov.action = action;
        prov.origin = KnowledgeOrigin::IndividualExperience;
        prov.channel = TransmissionChannel::Observation;
        prov.source_agent_id = agent_id;
        prov.generation_depth = 0;
        prov.transmission_fidelity = 1.0;
        prov.empirical_validation_score = 0.5; // Baseline upon discovery
        prov.acquisition_time = time;

        m_agent_knowledge[agent_id.raw()][concept_key] = prov;
    }

    void record_god_fly_lesson(EntityID agent_id, EntityID god_fly_id, const std::string& concept_key, AgentActionType action, double time) {
        KnowledgeProvenance prov;
        prov.concept_key = concept_key;
        prov.action = action;
        prov.origin = KnowledgeOrigin::GodFly;
        prov.channel = TransmissionChannel::Teaching;
        prov.source_agent_id = god_fly_id;
        prov.generation_depth = 0;
        prov.transmission_fidelity = 1.0;
        prov.empirical_validation_score = 0.25; // Initial hypothesis requires empirical practice
        prov.acquisition_time = time;

        m_agent_knowledge[agent_id.raw()][concept_key] = prov;
        m_transmission_events++;
    }

    bool transmit_peer_to_peer(EntityID learner_id, 
                               EntityID demonstrator_id, 
                               const std::string& concept_key, 
                               TransmissionChannel channel, 
                               double demonstrator_acuity,
                               double distance,
                               double time) 
    {
        auto it = m_agent_knowledge.find(demonstrator_id.raw());
        if (it == m_agent_knowledge.end()) return false;

        auto c_it = it->second.find(concept_key);
        if (c_it == it->second.end()) return false;

        const KnowledgeProvenance& source_prov = c_it->second;

        // Calculate transmission fidelity based on distance and acuity
        double base_decay = 0.95;
        if (channel == TransmissionChannel::Communication) base_decay = 0.98;
        else if (channel == TransmissionChannel::Imitation) base_decay = 0.90;

        double dist_factor = std::clamp(1.0 - (distance * 0.05), 0.5, 1.0);
        double hop_fidelity = source_prov.transmission_fidelity * base_decay * dist_factor * demonstrator_acuity;

        KnowledgeProvenance learner_prov;
        learner_prov.concept_key = concept_key;
        learner_prov.action = source_prov.action;
        learner_prov.channel = channel;
        learner_prov.source_agent_id = demonstrator_id;
        learner_prov.generation_depth = source_prov.generation_depth + 1;
        learner_prov.transmission_fidelity = hop_fidelity;
        learner_prov.empirical_validation_score = source_prov.empirical_validation_score * 0.5; // Unverified copy
        learner_prov.acquisition_time = time;

        // Origin classification: If passed over multiple generational hops, it constitutes cultural inheritance
        if (learner_prov.generation_depth >= 2 || source_prov.origin == KnowledgeOrigin::CulturalInheritance) {
            learner_prov.origin = KnowledgeOrigin::CulturalInheritance;
        } else {
            learner_prov.origin = KnowledgeOrigin::SocialTransfer;
        }

        m_agent_knowledge[learner_id.raw()][concept_key] = learner_prov;
        m_transmission_events++;
        return true;
    }

    void update_empirical_score(EntityID agent_id, const std::string& concept_key, double outcome_reward) {
        auto it = m_agent_knowledge.find(agent_id.raw());
        if (it != m_agent_knowledge.end()) {
            auto c_it = it->second.find(concept_key);
            if (c_it != it->second.end()) {
                c_it->second.empirical_validation_score = std::clamp(
                    c_it->second.empirical_validation_score * 0.8 + outcome_reward * 0.2, 
                    0.0, 1.0
                );
            }
        }
    }

    [[nodiscard]] const KnowledgeProvenance* get_provenance(EntityID agent_id, const std::string& concept_key) const noexcept {
        auto it = m_agent_knowledge.find(agent_id.raw());
        if (it != m_agent_knowledge.end()) {
            auto c_it = it->second.find(concept_key);
            if (c_it != it->second.end()) return &c_it->second;
        }
        return nullptr;
    }

    [[nodiscard]] size_t count_by_origin(KnowledgeOrigin origin) const noexcept {
        size_t count = 0;
        for (const auto& [_, concepts] : m_agent_knowledge) {
            for (const auto& [__, prov] : concepts) {
                if (prov.origin == origin) count++;
            }
        }
        return count;
    }

    [[nodiscard]] uint64_t total_transmission_events() const noexcept { return m_transmission_events; }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [raw_id, concepts] : m_agent_knowledge) {
            nlohmann::json agent_j = nlohmann::json::array();
            for (const auto& [_, prov] : concepts) {
                agent_j.push_back(prov.to_json());
            }
            j[std::to_string(raw_id)] = agent_j;
        }
        return {{"transmissions", m_transmission_events}, {"knowledge", j}};
    }

    void from_json(const nlohmann::json& j) {
        m_agent_knowledge.clear();
        if (j.contains("transmissions")) {
            m_transmission_events = j["transmissions"].get<uint64_t>();
        }
        if (j.contains("knowledge") && j["knowledge"].is_object()) {
            for (const auto& [id_str, concepts_j] : j["knowledge"].items()) {
                uint64_t raw_id = std::stoull(id_str);
                for (const auto& item : concepts_j) {
                    KnowledgeProvenance prov;
                    prov.concept_key = item.value("concept", "");
                    prov.action = static_cast<AgentActionType>(item.value("action", 0));
                    prov.origin = static_cast<KnowledgeOrigin>(item.value("origin_id", 0));
                    prov.channel = static_cast<TransmissionChannel>(item.value("channel_id", 0));
                    prov.source_agent_id = EntityID(item.value("source_id", 0ULL));
                    prov.generation_depth = item.value("gen_depth", 0U);
                    prov.transmission_fidelity = item.value("fidelity", 1.0);
                    prov.empirical_validation_score = item.value("empirical_score", 0.0);
                    prov.acquisition_time = item.value("time", 0.0);
                    m_agent_knowledge[raw_id][prov.concept_key] = prov;
                }
            }
        }
    }
};

} // namespace flgod::language
