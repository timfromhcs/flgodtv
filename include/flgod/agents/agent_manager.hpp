#pragma once

#include "flgod/agents/agent.hpp"
#include "flgod/agents/colony.hpp"
#include "flgod/world/world.hpp"
#include "flgod/core/rng.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

class AgentManager {
public:
    AgentManager() = default;

    void spawn_agent(const Agent& agent) {
        m_agents[agent.id()] = agent;
        if (agent.colony_id() > 0) {
            auto it = m_colonies.find(agent.colony_id());
            if (it != m_colonies.end()) {
                it->second.add_member(agent.id());
            }
        }
    }

    void remove_agent(EntityID id) {
        auto it = m_agents.find(id);
        if (it != m_agents.end()) {
            uint32_t cid = it->second.colony_id();
            if (cid > 0) {
                auto cit = m_colonies.find(cid);
                if (cit != m_colonies.end()) {
                    cit->second.remove_member(id);
                }
            }
            m_agents.erase(it);
        }
    }

    [[nodiscard]] bool has_agent(EntityID id) const noexcept {
        return m_agents.find(id) != m_agents.end();
    }

    [[nodiscard]] const Agent& get_agent(EntityID id) const {
        return m_agents.at(id);
    }

    [[nodiscard]] Agent& get_agent(EntityID id) {
        return m_agents.at(id);
    }

    [[nodiscard]] size_t agent_count() const noexcept { return m_agents.size(); }

    void register_colony(const Colony& colony) {
        m_colonies[colony.id()] = colony;
    }

    [[nodiscard]] const Colony& get_colony(uint32_t id) const {
        return m_colonies.at(id);
    }

    [[nodiscard]] Colony& get_colony(uint32_t id) {
        return m_colonies.at(id);
    }

    [[nodiscard]] size_t colony_count() const noexcept { return m_colonies.size(); }

    [[nodiscard]] const std::unordered_map<EntityID, Agent>& agents() const noexcept { return m_agents; }
    [[nodiscard]] const std::unordered_map<uint32_t, Colony>& colonies() const noexcept { return m_colonies; }

    // Multi-agent spatial step
    void step(double dt, const World& world, RNGStream& rng) {
        std::vector<EntityID> dead_agents;

        // Collect and sort agent IDs for bit-exact deterministic execution
        std::vector<EntityID> sorted_ids;
        sorted_ids.reserve(m_agents.size());
        for (const auto& [id, _] : m_agents) sorted_ids.push_back(id);
        std::sort(sorted_ids.begin(), sorted_ids.end());

        // Build list of active agent positions for peer perception (deterministic sorted order)
        std::vector<std::pair<EntityID, Vec3>> positions;
        positions.reserve(sorted_ids.size());
        for (EntityID id : sorted_ids) {
            const auto& agent = m_agents.at(id);
            if (agent.is_alive()) {
                positions.push_back({id, agent.position()});
            }
        }

        for (EntityID id : sorted_ids) {
            auto& agent = m_agents.at(id);
            if (!agent.is_alive()) {
                dead_agents.push_back(id);
                continue;
            }

            AgentSensoryInput input{};
            input.local_temperature = world.sample_temperature(agent.position().x, agent.position().z);
            input.local_wind = world.wind().sample(agent.position().x, agent.position().z);

            // Find nearest peer
            double min_peer_dist = 1e6;
            for (const auto& [other_id, other_pos] : positions) {
                if (other_id != id) {
                    double d = (other_pos - agent.position()).length();
                    if (d < min_peer_dist) {
                        min_peer_dist = d;
                        input.nearest_peer_id = other_id.raw();
                        input.peer_distance = d;
                        input.peer_detected = (d < 10.0 * agent.genome().sensory.visual_acuity);
                    }
                }
            }

            // Food perception: check colony storage or mock food patch
            if (agent.colony_id() > 0) {
                auto cit = m_colonies.find(agent.colony_id());
                if (cit != m_colonies.end() && cit->second.stored_resources() > 0.0) {
                    Vec3 nest_dir = cit->second.nest_position() - agent.position();
                    double nest_dist = nest_dir.length();
                    if (nest_dist < cit->second.territory_radius()) {
                        input.food_detected = true;
                        input.food_direction = nest_dir;
                        input.food_distance = nest_dist;
                    }
                }
            }

            // Execute agent step
            auto out = agent.step(dt, input, rng);

            // If agent successfully foraged in colony territory, deposit share into colony storage
            if (out.action == AgentActionType::Forage && agent.colony_id() > 0) {
                auto cit = m_colonies.find(agent.colony_id());
                if (cit != m_colonies.end()) {
                    cit->second.deposit_food(5.0);
                }
            }

            if (!agent.is_alive()) {
                dead_agents.push_back(id);
            }
        }

        // Clean up dead agents
        for (EntityID dead_id : dead_agents) {
            remove_agent(dead_id);
        }
    }

    [[nodiscard]] uint64_t compute_agents_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        // Sort IDs for deterministic hashing
        std::vector<uint64_t> ids;
        ids.reserve(m_agents.size());
        for (const auto& [id, _] : m_agents) ids.push_back(id.raw());
        std::sort(ids.begin(), ids.end());

        for (uint64_t raw_id : ids) {
            h ^= m_agents.at(EntityID(raw_id)).compute_hash();
            h *= 1099511628211ULL;
        }

        std::vector<uint32_t> col_ids;
        col_ids.reserve(m_colonies.size());
        for (const auto& [cid, _] : m_colonies) col_ids.push_back(cid);
        std::sort(col_ids.begin(), col_ids.end());

        for (uint32_t cid : col_ids) {
            h ^= m_colonies.at(cid).compute_hash();
            h *= 1099511628211ULL;
        }

        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json agents_arr = nlohmann::json::array();
        for (const auto& [_, a] : m_agents) agents_arr.push_back(a.to_json());
        nlohmann::json colonies_arr = nlohmann::json::array();
        for (const auto& [_, c] : m_colonies) colonies_arr.push_back(c.to_json());

        return {
            {"agents", agents_arr},
            {"colonies", colonies_arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_agents.clear();
        if (j.contains("agents")) {
            for (const auto& item : j["agents"]) {
                Agent a;
                a.from_json(item);
                m_agents[a.id()] = a;
            }
        }
        m_colonies.clear();
        if (j.contains("colonies")) {
            for (const auto& item : j["colonies"]) {
                Colony c;
                c.from_json(item);
                m_colonies[c.id()] = c;
            }
        }
    }

private:
    std::unordered_map<EntityID, Agent> m_agents;
    std::unordered_map<uint32_t, Colony> m_colonies;
};

} // namespace flgod
