#pragma once

#include "flgod/agents/agent_types.hpp"
#include "flgod/core/entity_id.hpp"
#include "flgod/evolution/genome.hpp"
#include "flgod/learning/memory.hpp"
#include "flgod/learning/learner.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <nlohmann/json.hpp>

namespace flgod {

class Agent {
public:
    Agent() = default;
    explicit Agent(EntityID id, uint32_t colony_id = 0, const Genome& genome = Genome{})
        : m_id(id), m_colony_id(colony_id), m_genome(genome) {}

    [[nodiscard]] EntityID id() const noexcept { return m_id; }
    [[nodiscard]] uint32_t colony_id() const noexcept { return m_colony_id; }
    void set_colony_id(uint32_t cid) noexcept { m_colony_id = cid; }

    [[nodiscard]] const Vec3& position() const noexcept { return m_position; }
    void set_position(const Vec3& pos) noexcept { m_position = pos; }

    [[nodiscard]] const Vec3& velocity() const noexcept { return m_velocity; }
    void set_velocity(const Vec3& vel) noexcept { m_velocity = vel; }

    [[nodiscard]] const Genome& genome() const noexcept { return m_genome; }
    Genome& genome() noexcept { return m_genome; }

    [[nodiscard]] const AgentDrives& drives() const noexcept { return m_drives; }
    AgentDrives& drives() noexcept { return m_drives; }

    [[nodiscard]] const MemorySystem& memory() const noexcept { return m_memory; }
    MemorySystem& memory() noexcept { return m_memory; }

    [[nodiscard]] const QLearner& learner() const noexcept { return m_learner; }
    QLearner& learner() noexcept { return m_learner; }

    [[nodiscard]] bool is_alive() const noexcept { return m_is_alive; }
    [[nodiscard]] uint32_t age_ticks() const noexcept { return m_age_ticks; }

    AgentActuatorOutput step(double dt, const AgentSensoryInput& input, RNGStream& rng) {
        if (!m_is_alive) return {};

        m_age_ticks++;

        // 1. Metabolic expenditure update
        double basal_drain = (m_genome.metabolism.basal_metabolic_rate + 
                             m_genome.brain.metabolic_cost_per_neuron * m_genome.brain.neuron_count) * dt;
        double motion_speed = m_velocity.length();
        double motion_drain = motion_speed * m_genome.body.mass * 0.1 * dt;
        double total_drain = basal_drain + motion_drain;

        m_drives.energy = std::max(0.0, m_drives.energy - total_drain);
        m_drives.hunger = std::min(100.0, m_drives.hunger + total_drain * 0.5);
        m_drives.fatigue = std::min(100.0, m_drives.fatigue + (motion_speed > 0.1 ? 0.2 : -0.1));
        m_drives.fatigue = std::max(0.0, m_drives.fatigue);

        // Starvation damage
        if (m_drives.energy <= 0.0) {
            m_drives.health = std::max(0.0, m_drives.health - 2.0 * dt);
            if (m_drives.health <= 0.0) {
                m_is_alive = false;
                return {};
            }
        }

        // 2. Behavioral arbitration and action selection
        AgentActuatorOutput out{};

        if (m_drives.fatigue > 85.0) {
            // Need rest
            out.action = AgentActionType::Rest;
            m_drives.fatigue = std::max(0.0, m_drives.fatigue - 5.0 * dt);
            m_velocity = m_velocity * 0.5;

        } else if (m_drives.hunger > 40.0 && input.food_detected) {
            if (input.food_distance < 1.0) {
                // In range to forage
                out.action = AgentActionType::Forage;
                double nutrition = 20.0 * m_genome.metabolism.energy_efficiency;
                m_drives.energy = std::min(100.0, m_drives.energy + nutrition);
                m_drives.hunger = std::max(0.0, m_drives.hunger - nutrition);
                m_drives.health = std::min(100.0, m_drives.health + 1.0);

                // Remember food source
                m_memory.episodic().record_event(m_id.raw(), m_age_ticks * dt, 1, static_cast<uint32_t>(AgentActionType::Forage), 5.0);
            } else {
                // Move toward food
                out.action = AgentActionType::Move;
                Vec3 dir = input.food_direction * (1.0 / (input.food_distance + 1e-6));
                double max_speed = 2.0 * m_genome.body.wing_span;
                out.movement_impulse = dir * (max_speed * dt);
                m_velocity = m_velocity + out.movement_impulse;
            }
        } else if (input.peer_detected && m_genome.social.social_curiosity > 0.5) {
            // Social interaction
            out.action = AgentActionType::Communicate;
            out.emitted_signal = static_cast<uint32_t>(m_id.raw() % 16);
            m_memory.social().update_interaction(input.nearest_peer_id, 0.5, m_age_ticks * dt);

        } else {
            // Random exploratory foraging
            out.action = AgentActionType::Move;
            double angle = rng.next_double() * 2.0 * 3.141592653589793;
            double speed = 1.0;
            out.movement_impulse = Vec3(std::cos(angle) * speed, 0.0, std::sin(angle) * speed) * dt;
            m_velocity = m_velocity + out.movement_impulse;
        }

        // Apply environmental wind drift
        m_velocity = m_velocity + input.local_wind * 0.1 * dt;
        m_position = m_position + m_velocity * dt;

        return out;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_double = [&h](double v) {
            uint64_t b = 0;
            std::memcpy(&b, &v, sizeof(double));
            h ^= b;
            h *= 1099511628211ULL;
        };
        h ^= m_id.raw();
        h ^= m_colony_id;
        mix_double(m_position.x);
        mix_double(m_position.y);
        mix_double(m_position.z);
        mix_double(m_drives.energy);
        mix_double(m_drives.hunger);
        h ^= m_genome.compute_hash();
        h ^= m_memory.compute_hash();
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", m_id.raw()},
            {"colony_id", m_colony_id},
            {"pos", {{"x", m_position.x}, {"y", m_position.y}, {"z", m_position.z}}},
            {"vel", {{"x", m_velocity.x}, {"y", m_velocity.y}, {"z", m_velocity.z}}},
            {"drives", m_drives.to_json()},
            {"genome", m_genome.to_json()},
            {"memory", m_memory.to_json()},
            {"learner", m_learner.to_json()},
            {"age", m_age_ticks},
            {"alive", m_is_alive}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_id = EntityID(j.value("id", 0ULL));
        m_colony_id = j.value("colony_id", 0u);
        if (j.contains("pos")) {
            m_position.x = j["pos"].value("x", 0.0);
            m_position.y = j["pos"].value("y", 0.0);
            m_position.z = j["pos"].value("z", 0.0);
        }
        if (j.contains("vel")) {
            m_velocity.x = j["vel"].value("x", 0.0);
            m_velocity.y = j["vel"].value("y", 0.0);
            m_velocity.z = j["vel"].value("z", 0.0);
        }
        if (j.contains("drives")) m_drives.from_json(j["drives"]);
        if (j.contains("genome")) m_genome.from_json(j["genome"]);
        if (j.contains("memory")) m_memory.from_json(j["memory"]);
        if (j.contains("learner")) m_learner.from_json(j["learner"]);
        m_age_ticks = j.value("age", 0u);
        m_is_alive = j.value("alive", true);
    }

private:
    EntityID m_id{NULL_ENTITY};
    uint32_t m_colony_id{0};
    Vec3 m_position{0.0, 0.0, 0.0};
    Vec3 m_velocity{0.0, 0.0, 0.0};
    Genome m_genome{};
    AgentDrives m_drives{};
    MemorySystem m_memory{};
    QLearner m_learner{};
    uint32_t m_age_ticks{0};
    bool m_is_alive{true};
};

} // namespace flgod
