#pragma once

#include "flgod/core/entity_id.hpp"
#include "flgod/world/fields.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

class Colony {
public:
    Colony() = default;
    explicit Colony(uint32_t id, const std::string& name, const Vec3& nest_pos, double territory_radius = 50.0)
        : m_id(id), m_name(name), m_nest_position(nest_pos), m_territory_radius(territory_radius) {}

    [[nodiscard]] uint32_t id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const Vec3& nest_position() const noexcept { return m_nest_position; }
    [[nodiscard]] double territory_radius() const noexcept { return m_territory_radius; }
    [[nodiscard]] double stored_resources() const noexcept { return m_stored_resources; }
    [[nodiscard]] const std::vector<EntityID>& members() const noexcept { return m_members; }
    [[nodiscard]] size_t population_count() const noexcept { return m_members.size(); }

    void add_member(EntityID agent_id) {
        if (std::find(m_members.begin(), m_members.end(), agent_id) == m_members.end()) {
            m_members.push_back(agent_id);
        }
    }

    void remove_member(EntityID agent_id) {
        auto it = std::find(m_members.begin(), m_members.end(), agent_id);
        if (it != m_members.end()) {
            m_members.erase(it);
        }
    }

    void deposit_food(double amount) noexcept {
        if (amount > 0.0) m_stored_resources += amount;
    }

    double withdraw_food(double requested_amount) noexcept {
        double granted = std::min(m_stored_resources, requested_amount);
        m_stored_resources -= granted;
        return granted;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        h ^= m_id;
        h *= 1099511628211ULL;
        uint64_t res_bits = 0;
        std::memcpy(&res_bits, &m_stored_resources, sizeof(double));
        h ^= res_bits;
        h *= 1099511628211ULL;
        for (const auto& m : m_members) {
            h ^= m.raw();
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json members_arr = nlohmann::json::array();
        for (const auto& m : m_members) {
            members_arr.push_back(m.raw());
        }
        return {
            {"id", m_id},
            {"name", m_name},
            {"nest", {{"x", m_nest_position.x}, {"y", m_nest_position.y}, {"z", m_nest_position.z}}},
            {"radius", m_territory_radius},
            {"stored", m_stored_resources},
            {"members", members_arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_id = j.value("id", 0u);
        m_name = j.value("name", "Colony");
        if (j.contains("nest")) {
            m_nest_position.x = j["nest"].value("x", 0.0);
            m_nest_position.y = j["nest"].value("y", 0.0);
            m_nest_position.z = j["nest"].value("z", 0.0);
        }
        m_territory_radius = j.value("radius", 50.0);
        m_stored_resources = j.value("stored", 0.0);
        m_members.clear();
        if (j.contains("members")) {
            for (const auto& item : j["members"]) {
                m_members.emplace_back(item.get<uint64_t>());
            }
        }
    }

private:
    uint32_t m_id{0};
    std::string m_name{"Colony"};
    Vec3 m_nest_position{0.0, 0.0, 0.0};
    double m_territory_radius{50.0};
    double m_stored_resources{0.0};
    std::vector<EntityID> m_members;
};

} // namespace flgod
