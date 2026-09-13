#pragma once

#include "flgod/world/fields.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <nlohmann/json.hpp>

namespace flgod {

struct AgentDrives {
    double energy{100.0};    // 0 to 100
    double hunger{0.0};      // 0 to 100 (high = starving)
    double fatigue{0.0};     // 0 to 100 (high = exhausted)
    double hydration{100.0}; // 0 to 100
    double health{100.0};    // 0 to 100

    [[nodiscard]] nlohmann::json to_json() const {
        uint64_t eb=0, hb=0, fb=0, hyb=0, hlb=0;
        std::memcpy(&eb, &energy, sizeof(double));
        std::memcpy(&hb, &hunger, sizeof(double));
        std::memcpy(&fb, &fatigue, sizeof(double));
        std::memcpy(&hyb, &hydration, sizeof(double));
        std::memcpy(&hlb, &health, sizeof(double));
        return {
            {"energy", energy}, {"hunger", hunger}, {"fatigue", fatigue},
            {"hydration", hydration}, {"health", health},
            {"drives_b", {eb, hb, fb, hyb, hlb}}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("drives_b")) {
            auto arr = j["drives_b"];
            uint64_t eb = arr[0], hb = arr[1], fb = arr[2], hyb = arr[3], hlb = arr[4];
            std::memcpy(&energy, &eb, sizeof(double));
            std::memcpy(&hunger, &hb, sizeof(double));
            std::memcpy(&fatigue, &fb, sizeof(double));
            std::memcpy(&hydration, &hyb, sizeof(double));
            std::memcpy(&health, &hlb, sizeof(double));
        } else {
            energy = j.value("energy", 100.0);
            hunger = j.value("hunger", 0.0);
            fatigue = j.value("fatigue", 0.0);
            hydration = j.value("hydration", 100.0);
            health = j.value("health", 100.0);
        }
    }
};

struct AgentSensoryInput {
    double local_temperature{20.0};
    Vec3 local_wind{0.0, 0.0, 0.0};
    bool food_detected{false};
    Vec3 food_direction{0.0, 0.0, 0.0};
    double food_distance{1e6};
    bool peer_detected{false};
    uint64_t nearest_peer_id{0};
    double peer_distance{1e6};
    uint32_t received_social_signal{0};
};

enum class AgentActionType : uint8_t {
    Idle = 0,
    Move = 1,
    Forage = 2,
    Drink = 3,
    Rest = 4,
    Communicate = 5
};

inline const char* action_type_to_string(AgentActionType act) noexcept {
    switch (act) {
        case AgentActionType::Idle: return "Idle";
        case AgentActionType::Move: return "Move";
        case AgentActionType::Forage: return "Forage";
        case AgentActionType::Drink: return "Drink";
        case AgentActionType::Rest: return "Rest";
        case AgentActionType::Communicate: return "Communicate";
        default: return "Unknown";
    }
}

struct AgentActuatorOutput {
    AgentActionType action{AgentActionType::Idle};
    Vec3 movement_impulse{0.0, 0.0, 0.0};
    uint32_t emitted_signal{0};
};

} // namespace flgod
