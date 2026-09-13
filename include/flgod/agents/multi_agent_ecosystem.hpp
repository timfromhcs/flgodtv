#pragma once

#include "flgod/core/world_state.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/llm/model_manager.hpp"
#include "flgod/llm/god_fly.hpp"
#include "flgod/llm/npc_system.hpp"
#include "flgod/language/vocabulary.hpp"
#include "flgod/language/social_learning.hpp"
#include "flgod/technology/programmable_world.hpp"
#include "flgod/brain/fly_brain_interface.hpp"
#include "flgod/brain/malecns_adapter.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace flgod {

struct MultiAgentEcosystemConfig {
    SimulationVersion version{CURRENT_SIMULATION_VERSION};
    RNGSeeds seeds{};
    double fixed_dt{1.0 / 60.0};
    uint64_t start_tick{0};
    double start_time{0.0};
    WorldConfig world_config{};
    std::string experiment_id{"multi_agent_experiment"};
    bool enable_god_fly{true};
    bool enable_tech_world{true};
};

class MultiAgentEcosystem {
public:
    MultiAgentEcosystem() = default;

    void initialize(const MultiAgentEcosystemConfig& config) {
        m_config = config;
        WorldStateConfig ws_config{
            .version = config.version,
            .seeds = config.seeds,
            .fixed_dt = config.fixed_dt,
            .start_tick = config.start_tick,
            .start_time = config.start_time,
            .world_config = config.world_config
        };
        m_world_state = WorldState(ws_config);

        // Initialize LLM subsystem
        m_model_mgr = std::make_unique<llm::ModelManager>();
        m_model_mgr->load_model(llm::ModelRole::GodFly, "models/god_fly_instruct.gguf", llm::LLMBackend::CPU, /*allow_mock_fallback=*/true);
        m_model_mgr->load_model(llm::ModelRole::NPC, "models/npc_dialogue.gguf", llm::LLMBackend::CPU, /*allow_mock_fallback=*/true);

        // Initialize God Fly and NPC manager
        m_god_fly = std::make_unique<llm::GodFly>(m_model_mgr.get());
        m_npc_mgr = std::make_unique<llm::NPCManager>(m_model_mgr.get());

        // VocabularySystem already pre-populates foundational words (101: ACT_FORAGE, 201: OBJ_NECTAR)
        language::SymbolicUtterance init_utt{};
        init_utt.sequence = {101, 201};
        m_vocab.record_utterance(init_utt);

        m_is_initialized = true;
    }

    [[nodiscard]] bool is_initialized() const noexcept { return m_is_initialized; }
    [[nodiscard]] const WorldState& world_state() const noexcept { return m_world_state; }
    [[nodiscard]] WorldState& world_state() noexcept { return m_world_state; }
    [[nodiscard]] const AgentManager& agent_manager() const noexcept { return m_agent_mgr; }
    [[nodiscard]] AgentManager& agent_manager() noexcept { return m_agent_mgr; }
    [[nodiscard]] llm::GodFly* god_fly() noexcept { return m_god_fly.get(); }
    [[nodiscard]] llm::NPCManager* npc_manager() noexcept { return m_npc_mgr.get(); }
    [[nodiscard]] language::SocialLearningTracker& social_learning() noexcept { return m_social_learning; }
    [[nodiscard]] const language::SocialLearningTracker& social_learning() const noexcept { return m_social_learning; }
    [[nodiscard]] language::VocabularySystem& vocabulary() noexcept { return m_vocab; }
    [[nodiscard]] technology::ProgrammableTechnologyLayer& tech_world() noexcept { return m_tech_world; }

    void attach_brain(EntityID agent_id, std::shared_ptr<brain::IFlyBrain> brain) {
        m_agent_brains[agent_id.raw()] = std::move(brain);
    }

    [[nodiscard]] brain::IFlyBrain* get_brain(EntityID agent_id) noexcept {
        auto it = m_agent_brains.find(agent_id.raw());
        if (it != m_agent_brains.end()) return it->second.get();
        return nullptr;
    }

    // Step the entire multi-agent ecosystem
    SimulationStep step() {
        if (!m_is_initialized) {
            throw std::runtime_error("MultiAgentEcosystem::step() called on uninitialized ecosystem!");
        }

        // 1. Clock step
        SimulationStep s = m_world_state.clock().step();
        double dt = s.dt;
        double current_time = s.elapsed_seconds;

        // 2. World & Weather step
        m_world_state.world().step(dt, current_time);

        // 3. Physics step
        m_world_state.physics().step(dt);

        // 4. Connectome brain updates & motor translation for equipped agents
        for (auto& [raw_id, brain_ptr] : m_agent_brains) {
            EntityID id(raw_id);
            if (!m_agent_mgr.has_agent(id)) continue;

            Agent& agent = m_agent_mgr.get_agent(id);
            if (!agent.is_alive()) continue;

            // Biological sensory inputs: sample world fields
            brain::BrainSensoryInput sensory{};
            sensory.air_velocity = m_world_state.world().wind().sample(agent.position().x, agent.position().z);
            sensory.internal_energy = agent.drives().energy;
            sensory.internal_fatigue = agent.drives().fatigue;

            // Odor stimulus near food / nectar
            if (agent.colony_id() > 0) {
                const auto& col = m_agent_mgr.get_colony(agent.colony_id());
                double d = (col.nest_position() - agent.position()).length();
                if (d < col.territory_radius()) {
                    sensory.odor_sugar_intensity = std::clamp(1.0 - (d / col.territory_radius()), 0.0, 1.0);
                }
            }

            // Step connectome biological somas
            brain::BrainMotorOutput motor{};
            brain_ptr->step(dt, sensory, motor);

            // Apply connectome motor output to agent kinematics
            Vec3 pos = agent.position();
            double freq_diff = (motor.right_wing_freq_hz - motor.left_wing_freq_hz) / 50.0;
            double heading_change = freq_diff * dt * 2.0;
            double avg_freq = (motor.left_wing_freq_hz + motor.right_wing_freq_hz) * 0.5;
            double forward_thrust = (avg_freq / 200.0) * (motor.wing_amplitude_deg / 150.0) * dt * 5.0;

            pos.x += forward_thrust * std::cos(heading_change);
            pos.z += forward_thrust * std::sin(heading_change);
            agent.set_position(pos);

            // Proboscis extension triggers feeding if sugar odor present
            if (motor.proboscis_extension > 0.5 && sensory.odor_sugar_intensity > 0.1) {
                double nutrition = sensory.odor_sugar_intensity * 10.0;
                agent.drives().energy = std::min(100.0, agent.drives().energy + nutrition);
                agent.drives().hunger = std::max(0.0, agent.drives().hunger - nutrition);
            }
        }

        // 5. Multi-agent spatial & behavioral step
        m_agent_mgr.step(dt, m_world_state.world(), m_world_state.rng().agent());

        // 6. Social communication & peer transmission among nearby agents
        std::vector<EntityID> agent_ids;
        for (const auto& [raw_id, _] : m_agent_brains) agent_ids.push_back(EntityID(raw_id));

        for (size_t i = 0; i < agent_ids.size(); ++i) {
            if (!m_agent_mgr.has_agent(agent_ids[i])) continue;
            const Agent& a1 = m_agent_mgr.get_agent(agent_ids[i]);
            if (!a1.is_alive()) continue;

            for (size_t j = i + 1; j < agent_ids.size(); ++j) {
                if (!m_agent_mgr.has_agent(agent_ids[j])) continue;
                const Agent& a2 = m_agent_mgr.get_agent(agent_ids[j]);
                if (!a2.is_alive()) continue;

                double dist = (a1.position() - a2.position()).length();
                if (dist < 15.0) {
                    m_social_learning.transmit_peer_to_peer(
                        a2.id(), a1.id(), "FORAGE_LOC",
                        language::TransmissionChannel::Communication,
                        a1.genome().sensory.visual_acuity,
                        dist, current_time
                    );
                }
            }
        }

        // 7. God Fly teacher evaluation & lesson dispatch
        if (m_god_fly && m_config.enable_god_fly) {
            for (const auto& aid : agent_ids) {
                if (!m_agent_mgr.has_agent(aid)) continue;
                Agent& student = m_agent_mgr.get_agent(aid);
                if (!student.is_alive()) continue;

                if (student.drives().energy < 40.0 || student.drives().hunger > 60.0) {
                    llm::TeachingPacket packet{};
                    if (m_god_fly->create_lesson(
                            llm::TeachingMode::Suggestion,
                            student.id(),
                            "FORAGE_NECTAR",
                            "Energy critical. Forage near colony nest.",
                            packet,
                            &m_agent_mgr)) 
                    {
                        m_god_fly->deliver_lesson(packet, student, current_time);
                        m_social_learning.record_god_fly_lesson(
                            student.id(), m_god_fly->get_id(), "FORAGE_NECTAR",
                            AgentActionType::Forage, current_time
                        );
                    }
                    break;
                }
            }
        }

        // 8. NPC event-driven inference
        if (m_npc_mgr) {
            for (const auto& aid : agent_ids) {
                if (!m_agent_mgr.has_agent(aid)) continue;
                Agent& agent = m_agent_mgr.get_agent(aid);
                if (!agent.is_alive()) continue;

                if (agent.drives().energy < 20.0) {
                    llm::ValidatedLLMAction action{};
                    m_npc_mgr->trigger_event_inference(
                        aid,
                        llm::NPCInferenceTrigger::CriticalResource,
                        current_time,
                        "Energy depletion warning",
                        action,
                        &m_agent_mgr
                    );
                }
            }
        }

        // 9. Programmable technology layer step (VMs, network packets, actuators)
        if (m_config.enable_tech_world) {
            m_tech_world.step(dt);
        }

        return s;
    }

    void run_ticks(uint64_t count) {
        for (uint64_t i = 0; i < count; ++i) {
            step();
        }
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = m_world_state.compute_hash();

        // Agents & Colonies
        h ^= m_agent_mgr.compute_agents_hash();
        h *= 1099511628211ULL;

        // Social learning transmissions
        h ^= (static_cast<uint64_t>(m_social_learning.total_transmission_events()) + 0x9e3779b97f4a7c15ULL);
        h *= 1099511628211ULL;

        // Programmable world
        h ^= m_tech_world.compute_hash();
        h *= 1099511628211ULL;

        // God Fly lesson count
        if (m_god_fly) {
            h ^= (m_god_fly->get_total_lessons_taught() + 0x517cc1b727220a95ULL);
            h *= 1099511628211ULL;
        }

        // Brain hashes
        std::vector<uint64_t> brain_ids;
        for (const auto& [id, _] : m_agent_brains) brain_ids.push_back(id);
        std::sort(brain_ids.begin(), brain_ids.end());
        for (uint64_t bid : brain_ids) {
            h ^= m_agent_brains.at(bid)->compute_brain_hash();
            h *= 1099511628211ULL;
        }

        return h;
    }

    [[nodiscard]] nlohmann::json create_checkpoint() const {
        nlohmann::json j;
        j["version"] = m_config.version.to_string();
        j["experiment_id"] = m_config.experiment_id;
        j["world_state"] = m_world_state.to_json();
        j["agent_manager"] = m_agent_mgr.to_json();
        j["social_learning"] = m_social_learning.to_json();
        j["tech_world"] = m_tech_world.to_json();

        nlohmann::json brains_j = nlohmann::json::object();
        for (const auto& [bid, brain_ptr] : m_agent_brains) {
            brains_j[std::to_string(bid)] = brain_ptr->to_json();
        }
        j["brains"] = brains_j;

        j["checkpoint_hash"] = compute_hash();
        return j;
    }

    void restore_checkpoint(const nlohmann::json& j) {
        if (!j.contains("world_state") || !j.contains("agent_manager")) {
            throw std::runtime_error("Invalid MultiAgentEcosystem checkpoint JSON!");
        }

        m_world_state.from_json(j["world_state"]);
        m_agent_mgr.from_json(j["agent_manager"]);

        if (j.contains("social_learning")) {
            m_social_learning.from_json(j["social_learning"]);
        }
        if (j.contains("tech_world")) {
            m_tech_world.from_json(j["tech_world"]);
        }
        if (j.contains("brains") && j["brains"].is_object()) {
            for (const auto& [bid_str, b_json] : j["brains"].items()) {
                uint64_t bid = std::stoull(bid_str);
                auto it = m_agent_brains.find(bid);
                if (it != m_agent_brains.end() && it->second) {
                    it->second->from_json(b_json);
                }
            }
        }

        m_is_initialized = true;
    }

private:
    MultiAgentEcosystemConfig m_config;
    WorldState m_world_state;
    AgentManager m_agent_mgr;
    std::unique_ptr<llm::ModelManager> m_model_mgr;
    std::unique_ptr<llm::GodFly> m_god_fly;
    std::unique_ptr<llm::NPCManager> m_npc_mgr;
    language::VocabularySystem m_vocab;
    language::SocialLearningTracker m_social_learning;
    technology::ProgrammableTechnologyLayer m_tech_world;
    std::unordered_map<uint64_t, std::shared_ptr<brain::IFlyBrain>> m_agent_brains;
    bool m_is_initialized{false};
};

} // namespace flgod
