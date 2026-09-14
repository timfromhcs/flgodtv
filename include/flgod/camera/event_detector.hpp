#pragma once

#include "flgod/core/entity_id.hpp"
#include "flgod/physics/physics_types.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace flgod {

enum class SimulationEventType : uint32_t {
    AgentBirth = 0,
    AgentDeath = 1,
    Collision = 2,
    ForageSuccess = 3,
    SocialInteraction = 4,
    LessonTaught = 5,
    TechExecution = 6,
    SpeciationDivergence = 7,
    WeatherShift = 8,
    BrainSurge = 9,
    Combat = 10,
    Predation = 11,
    Discovery = 12,
    ResourceDiscovery = 13,
    Construction = 14,
    Migration = 15,
    Fire = 16,
    Flooding = 17,
    UnusualBehavior = 18,
    Generic = 19
};

[[nodiscard]] inline const char* to_string(SimulationEventType type) noexcept {
    switch (type) {
        case SimulationEventType::AgentBirth: return "AgentBirth";
        case SimulationEventType::AgentDeath: return "AgentDeath";
        case SimulationEventType::Collision: return "Collision";
        case SimulationEventType::ForageSuccess: return "ForageSuccess";
        case SimulationEventType::SocialInteraction: return "SocialInteraction";
        case SimulationEventType::LessonTaught: return "LessonTaught";
        case SimulationEventType::TechExecution: return "TechExecution";
        case SimulationEventType::SpeciationDivergence: return "SpeciationDivergence";
        case SimulationEventType::WeatherShift: return "WeatherShift";
        case SimulationEventType::BrainSurge: return "BrainSurge";
        case SimulationEventType::Combat: return "Combat";
        case SimulationEventType::Predation: return "Predation";
        case SimulationEventType::Discovery: return "Discovery";
        case SimulationEventType::ResourceDiscovery: return "ResourceDiscovery";
        case SimulationEventType::Construction: return "Construction";
        case SimulationEventType::Migration: return "Migration";
        case SimulationEventType::Fire: return "Fire";
        case SimulationEventType::Flooding: return "Flooding";
        case SimulationEventType::UnusualBehavior: return "UnusualBehavior";
        case SimulationEventType::Generic: default: return "Generic";
    }
}

struct SimulationEvent {
    uint64_t id{0};
    uint64_t tick{0};
    SimulationEventType type{SimulationEventType::Generic};
    EntityID source_entity{NULL_ENTITY};
    EntityID target_entity{NULL_ENTITY};
    Vec3 position{0.0, 0.0, 0.0};
    double priority{0.0};
    uint32_t lifetime_ticks{60};
    uint32_t age_ticks{0};
    bool presentation_eligible{true};
    std::string description;

    [[nodiscard]] bool is_expired() const noexcept {
        return age_ticks >= lifetime_ticks;
    }

    // Deterministic priority ordering:
    // 1. Highest priority first
    // 2. Earliest tick first
    // 3. Lowest ID first (strictly stable tie-breaking)
    bool operator<(const SimulationEvent& other) const noexcept {
        if (std::abs(priority - other.priority) > 1e-9) {
            return priority < other.priority; // for max-heap or sort ascending
        }
        if (tick != other.tick) {
            return tick > other.tick; // earlier tick is higher priority
        }
        return id > other.id; // smaller ID is higher priority
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", id},
            {"tick", tick},
            {"type", static_cast<uint32_t>(type)},
            {"type_name", to_string(type)},
            {"source_entity", source_entity.raw()},
            {"target_entity", target_entity.raw()},
            {"pos", {position.x, position.y, position.z}},
            {"priority", priority},
            {"lifetime_ticks", lifetime_ticks},
            {"age_ticks", age_ticks},
            {"presentation_eligible", presentation_eligible},
            {"description", description}
        };
    }

    static SimulationEvent from_json(const nlohmann::json& j) {
        SimulationEvent ev;
        ev.id = j.value("id", 0ULL);
        ev.tick = j.value("tick", 0ULL);
        ev.type = static_cast<SimulationEventType>(j.value("type", static_cast<uint32_t>(SimulationEventType::Generic)));
        ev.source_entity = EntityID(j.value("source_entity", 0ULL));
        ev.target_entity = EntityID(j.value("target_entity", 0ULL));
        if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() >= 3) {
            ev.position = Vec3{j["pos"][0].get<double>(), j["pos"][1].get<double>(), j["pos"][2].get<double>()};
        }
        ev.priority = j.value("priority", 0.0);
        ev.lifetime_ticks = j.value("lifetime_ticks", 60U);
        ev.age_ticks = j.value("age_ticks", 0U);
        ev.presentation_eligible = j.value("presentation_eligible", true);
        ev.description = j.value("description", "");
        return ev;
    }
};

class EventDetector {
public:
    explicit EventDetector(size_t max_history = 1000) : m_max_history(max_history) {}

    uint64_t record_event(SimulationEvent ev) {
        ev.id = ++m_next_event_id;
        m_active_events.push_back(ev);
        sort_active_events();

        if (m_event_history.size() >= m_max_history) {
            m_event_history.erase(m_event_history.begin());
        }
        m_event_history.push_back(ev);
        return ev.id;
    }

    void step(uint64_t current_tick) {
        m_current_tick = current_tick;
        std::vector<SimulationEvent> remaining;
        remaining.reserve(m_active_events.size());

        for (auto& ev : m_active_events) {
            ev.age_ticks++;
            if (!ev.is_expired()) {
                remaining.push_back(ev);
            }
        }
        m_active_events = std::move(remaining);
        sort_active_events();
    }

    void clear() noexcept {
        m_active_events.clear();
        m_event_history.clear();
        m_next_event_id = 0;
        m_current_tick = 0;
    }

    [[nodiscard]] const std::vector<SimulationEvent>& active_events() const noexcept {
        return m_active_events;
    }

    [[nodiscard]] const std::vector<SimulationEvent>& event_history() const noexcept {
        return m_event_history;
    }

    [[nodiscard]] const SimulationEvent* highest_priority_event() const noexcept {
        for (const auto& ev : m_active_events) {
            if (ev.presentation_eligible && !ev.is_expired()) {
                return &ev;
            }
        }
        return nullptr;
    }

    [[nodiscard]] size_t active_count() const noexcept { return m_active_events.size(); }
    [[nodiscard]] size_t history_count() const noexcept { return m_event_history.size(); }
    [[nodiscard]] uint64_t current_tick() const noexcept { return m_current_tick; }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json active_json = nlohmann::json::array();
        for (const auto& ev : m_active_events) {
            active_json.push_back(ev.to_json());
        }
        return {
            {"next_event_id", m_next_event_id},
            {"current_tick", m_current_tick},
            {"active_events", active_json}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_next_event_id = j.value("next_event_id", 0ULL);
        m_current_tick = j.value("current_tick", 0ULL);
        m_active_events.clear();
        if (j.contains("active_events") && j["active_events"].is_array()) {
            for (const auto& item : j["active_events"]) {
                m_active_events.push_back(SimulationEvent::from_json(item));
            }
        }
        sort_active_events();
    }

private:
    void sort_active_events() {
        // Sort descending by priority: top event is at index 0
        std::sort(m_active_events.begin(), m_active_events.end(), [](const SimulationEvent& a, const SimulationEvent& b) {
            // b < a means a is higher priority than b
            return b < a;
        });
    }

    size_t m_max_history{1000};
    uint64_t m_next_event_id{0};
    uint64_t m_current_tick{0};
    std::vector<SimulationEvent> m_active_events;
    std::vector<SimulationEvent> m_event_history;
};

} // namespace flgod
