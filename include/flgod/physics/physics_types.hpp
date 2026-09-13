#pragma once

#include "flgod/core/entity_id.hpp"
#include "flgod/world/fields.hpp" // For Vec3
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <nlohmann/json.hpp>

namespace flgod {

enum class SimulationFidelity : uint8_t {
    L0_Full = 0,       // Full rigid body, inter-object collision, constraints, CCD
    L1_Reduced = 1,    // Simplified drag, ground collision only, skip pair narrowphase
    L2_Statistical = 2 // Statistical drift, regional spatial density
};

inline const char* fidelity_to_string(SimulationFidelity f) noexcept {
    switch (f) {
        case SimulationFidelity::L0_Full: return "L0_Full";
        case SimulationFidelity::L1_Reduced: return "L1_Reduced";
        case SimulationFidelity::L2_Statistical: return "L2_Statistical";
        default: return "Unknown";
    }
}

enum class BodyMotionType : uint8_t {
    Static = 0,
    Kinematic = 1,
    Dynamic = 2
};

enum class ShapeType : uint8_t {
    Sphere = 0,
    Box = 1,
    Capsule = 2,
    Plane = 3
};

struct CollisionShape {
    ShapeType type{ShapeType::Sphere};
    double radius{0.5}; // For sphere and capsule
    Vec3 half_extents{0.5, 0.5, 0.5}; // For box
    double height{1.0}; // For capsule

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"type", static_cast<uint8_t>(type)},
            {"radius", radius},
            {"half_extents", {{"x", half_extents.x}, {"y", half_extents.y}, {"z", half_extents.z}}},
            {"height", height}
        };
    }

    void from_json(const nlohmann::json& j) {
        type = static_cast<ShapeType>(j.value("type", static_cast<uint8_t>(0)));
        radius = j.value("radius", 0.5);
        if (j.contains("half_extents")) {
            half_extents.x = j["half_extents"].value("x", 0.5);
            half_extents.y = j["half_extents"].value("y", 0.5);
            half_extents.z = j["half_extents"].value("z", 0.5);
        }
        height = j.value("height", 1.0);
    }
};

struct RigidBodyState {
    EntityID id{NULL_ENTITY};
    Vec3 position{0.0, 0.0, 0.0};
    Vec3 linear_velocity{0.0, 0.0, 0.0};
    Vec3 rotation{0.0, 0.0, 0.0}; // Euler angles in radians
    Vec3 angular_velocity{0.0, 0.0, 0.0};
    double mass{1.0};             // kg (0 for static)
    double friction{0.5};         // [0.0, 1.0]
    double restitution{0.2};      // bounciness [0.0, 1.0]
    double linear_damping{0.05};  // air resistance
    double gravity_factor{1.0};
    BodyMotionType motion_type{BodyMotionType::Dynamic};
    SimulationFidelity fidelity{SimulationFidelity::L0_Full};
    bool is_active{true};
    bool ccd_enabled{false};
    CollisionShape shape{};

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_double = [&h](double v) {
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(double));
            h ^= bits;
            h *= 1099511628211ULL;
        };
        h ^= id.raw();
        h *= 1099511628211ULL;
        mix_double(position.x);
        mix_double(position.y);
        mix_double(position.z);
        mix_double(linear_velocity.x);
        mix_double(linear_velocity.y);
        mix_double(linear_velocity.z);
        mix_double(mass);
        h ^= static_cast<uint8_t>(motion_type);
        h ^= static_cast<uint8_t>(fidelity);
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", id.raw()},
            {"pos", {{"x", position.x}, {"y", position.y}, {"z", position.z}}},
            {"vel", {{"x", linear_velocity.x}, {"y", linear_velocity.y}, {"z", linear_velocity.z}}},
            {"mass", mass},
            {"friction", friction},
            {"restitution", restitution},
            {"motion_type", static_cast<uint8_t>(motion_type)},
            {"fidelity", static_cast<uint8_t>(fidelity)},
            {"is_active", is_active},
            {"shape", shape.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        id = EntityID(j.value("id", 0ULL));
        if (j.contains("pos")) {
            position.x = j["pos"].value("x", 0.0);
            position.y = j["pos"].value("y", 0.0);
            position.z = j["pos"].value("z", 0.0);
        }
        if (j.contains("vel")) {
            linear_velocity.x = j["vel"].value("x", 0.0);
            linear_velocity.y = j["vel"].value("y", 0.0);
            linear_velocity.z = j["vel"].value("z", 0.0);
        }
        mass = j.value("mass", 1.0);
        friction = j.value("friction", 0.5);
        restitution = j.value("restitution", 0.2);
        motion_type = static_cast<BodyMotionType>(j.value("motion_type", 2));
        fidelity = static_cast<SimulationFidelity>(j.value("fidelity", 0));
        is_active = j.value("is_active", true);
        if (j.contains("shape")) {
            shape.from_json(j["shape"]);
        }
    }
};

enum class ConstraintType : uint8_t {
    Distance = 0,
    Point = 1,
    Hinge = 2
};

struct PhysicsConstraint {
    uint64_t id{0};
    EntityID body_a{NULL_ENTITY};
    EntityID body_b{NULL_ENTITY};
    ConstraintType type{ConstraintType::Distance};
    double target_distance{1.0};
    double break_force{1000.0}; // Structural failure threshold in Newtons
    bool is_broken{false};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", id},
            {"body_a", body_a.raw()},
            {"body_b", body_b.raw()},
            {"type", static_cast<uint8_t>(type)},
            {"dist", target_distance},
            {"break_force", break_force},
            {"is_broken", is_broken}
        };
    }

    void from_json(const nlohmann::json& j) {
        id = j.value("id", 0ULL);
        body_a = EntityID(j.value("body_a", 0ULL));
        body_b = EntityID(j.value("body_b", 0ULL));
        type = static_cast<ConstraintType>(j.value("type", 0));
        target_distance = j.value("dist", 1.0);
        break_force = j.value("break_force", 1000.0);
        is_broken = j.value("is_broken", false);
    }
};

struct RaycastHit {
    bool hit{false};
    EntityID body_id{NULL_ENTITY};
    Vec3 point{0.0, 0.0, 0.0};
    Vec3 normal{0.0, 1.0, 0.0};
    double distance{0.0};
};

} // namespace flgod
