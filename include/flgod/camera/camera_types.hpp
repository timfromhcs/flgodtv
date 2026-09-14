#pragma once

#include "flgod/core/entity_id.hpp"
#include "flgod/physics/physics_types.hpp"
#include <cstdint>
#include <string>
#include <cmath>
#include <nlohmann/json.hpp>

namespace flgod {

// GEMINI.md Section 81: Actual reusable shot types
enum class ShotType : uint32_t {
    Macro = 0,
    Close = 1,
    Medium = 2,
    Wide = 3,
    Establishing = 4,
    Tracking = 5,
    Orbit = 6,
    Overhead = 7,
    LowAngle = 8,
    POV = 9,
    ReactionShot = 10
};

[[nodiscard]] inline const char* to_string(ShotType shot) noexcept {
    switch (shot) {
        case ShotType::Macro: return "Macro";
        case ShotType::Close: return "Close";
        case ShotType::Medium: return "Medium";
        case ShotType::Wide: return "Wide";
        case ShotType::Establishing: return "Establishing";
        case ShotType::Tracking: return "Tracking";
        case ShotType::Orbit: return "Orbit";
        case ShotType::Overhead: return "Overhead";
        case ShotType::LowAngle: return "LowAngle";
        case ShotType::POV: return "POV";
        case ShotType::ReactionShot: return "ReactionShot";
        default: return "Medium";
    }
}

// GEMINI.md Section 83: Four independent camera directors
enum class CameraChannel : uint32_t {
    Cam1_GodFly = 0,
    Cam2_LearningAgent = 1,
    Cam3_Event = 2,
    Cam4_EnvironmentColony = 3
};

[[nodiscard]] inline const char* to_string(CameraChannel ch) noexcept {
    switch (ch) {
        case CameraChannel::Cam1_GodFly: return "Cam1_GodFly";
        case CameraChannel::Cam2_LearningAgent: return "Cam2_LearningAgent";
        case CameraChannel::Cam3_Event: return "Cam3_Event";
        case CameraChannel::Cam4_EnvironmentColony: return "Cam4_EnvironmentColony";
        default: return "Unknown";
    }
}

struct CameraPose {
    Vec3 position{0.0, 10.0, 20.0};
    Vec3 look_at{0.0, 0.0, 0.0};
    Vec3 up{0.0, 1.0, 0.0};
    float fov{60.0f};
    float distance{10.0f};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"pos", {position.x, position.y, position.z}},
            {"look_at", {look_at.x, look_at.y, look_at.z}},
            {"up", {up.x, up.y, up.z}},
            {"fov", fov},
            {"distance", distance}
        };
    }

    static CameraPose from_json(const nlohmann::json& j) {
        CameraPose p;
        if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() >= 3) {
            p.position = Vec3{j["pos"][0].get<double>(), j["pos"][1].get<double>(), j["pos"][2].get<double>()};
        }
        if (j.contains("look_at") && j["look_at"].is_array() && j["look_at"].size() >= 3) {
            p.look_at = Vec3{j["look_at"][0].get<double>(), j["look_at"][1].get<double>(), j["look_at"][2].get<double>()};
        }
        if (j.contains("up") && j["up"].is_array() && j["up"].size() >= 3) {
            p.up = Vec3{j["up"][0].get<double>(), j["up"][1].get<double>(), j["up"][2].get<double>()};
        }
        p.fov = j.value("fov", 60.0f);
        p.distance = j.value("distance", 10.0f);
        return p;
    }
};

struct CameraTarget {
    bool is_valid{false};
    EntityID entity_id{NULL_ENTITY};
    Vec3 position{0.0, 0.0, 0.0};
    Vec3 velocity{0.0, 0.0, 0.0};
    float bounding_radius{1.0f};
    double priority{0.0};
    ShotType suggested_shot{ShotType::Medium};
    uint64_t source_event_id{0};
    std::string label;

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"valid", is_valid},
            {"entity_id", entity_id.raw()},
            {"pos", {position.x, position.y, position.z}},
            {"vel", {velocity.x, velocity.y, velocity.z}},
            {"radius", bounding_radius},
            {"priority", priority},
            {"shot", static_cast<uint32_t>(suggested_shot)},
            {"shot_name", to_string(suggested_shot)},
            {"event_id", source_event_id},
            {"label", label}
        };
    }

    static CameraTarget from_json(const nlohmann::json& j) {
        CameraTarget t;
        t.is_valid = j.value("valid", true);
        t.entity_id = EntityID(j.value("entity_id", 0ULL));
        if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() >= 3) {
            t.position = Vec3{j["pos"][0].get<double>(), j["pos"][1].get<double>(), j["pos"][2].get<double>()};
        }
        if (j.contains("vel") && j["vel"].is_array() && j["vel"].size() >= 3) {
            t.velocity = Vec3{j["vel"][0].get<double>(), j["vel"][1].get<double>(), j["vel"][2].get<double>()};
        }
        t.bounding_radius = j.value("radius", 1.0f);
        t.priority = j.value("priority", 0.0);
        t.suggested_shot = static_cast<ShotType>(j.value("shot", static_cast<uint32_t>(ShotType::Medium)));
        t.source_event_id = j.value("event_id", 0ULL);
        t.label = j.value("label", "");
        return t;
    }
};

} // namespace flgod
