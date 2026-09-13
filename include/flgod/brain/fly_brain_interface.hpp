#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <nlohmann/json.hpp>
#include "flgod/world/fields.hpp"

namespace flgod::brain {

struct NeuronSoma {
    uint64_t body_id{0};
    uint64_t nucleus_id{0};
    double x_um{0.0};  // Micrometers
    double y_um{0.0};
    double z_um{0.0};
    uint32_t body_size{0};
    bool is_left_hemisphere{true};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"body_id", body_id},
            {"x", x_um}, {"y", y_um}, {"z", z_um},
            {"size", body_size},
            {"left", is_left_hemisphere}
        };
    }
};

struct BrainSensoryInput {
    // Compound visual input: Left and Right optical intensity fields (normalized 0.0 to 1.0)
    std::array<double, 8> left_eye_sectors{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 8> right_eye_sectors{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    
    // Antennal olfactory sensation (sugar, pheromone, CO2/danger)
    double odor_sugar_intensity{0.0};
    double odor_pheromone_intensity{0.0};
    double odor_danger_intensity{0.0};

    // Mechanosensory wind perception
    Vec3 air_velocity{0.0, 0.0, 0.0};

    // Internal physiological interoception
    double internal_energy{100.0};
    double internal_fatigue{0.0};
};

struct BrainMotorOutput {
    // Flight motor: Left and right wingbeat frequencies (Hz) & amplitude
    double left_wing_freq_hz{0.0};   // e.g. 200-250 Hz
    double right_wing_freq_hz{0.0};
    double wing_amplitude_deg{0.0};  // Stroke amplitude (e.g. 120-150 deg)

    // Steering torque yaw/pitch/roll
    Vec3 steering_torque{0.0, 0.0, 0.0};

    // Feeding proboscis extension response (PER) [0.0 = retracted, 1.0 = fully extended]
    double proboscis_extension{0.0};

    // Leg locomotion stepping impulse
    Vec3 leg_locomotion_impulse{0.0, 0.0, 0.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"left_wing_freq", left_wing_freq_hz},
            {"right_wing_freq", right_wing_freq_hz},
            {"amplitude", wing_amplitude_deg},
            {"proboscis", proboscis_extension}
        };
    }
};

// SECTION 8: "Create an adapter boundary: FLGOD CORE -> FlyBrain Interface -> Existing Fly-Brain"
// "The Fly-Brain adapter should be isolated from: world, physics, learning, evolution, rendering, LLM"
class IFlyBrain {
public:
    virtual ~IFlyBrain() = default;

    // Execute one neural step
    virtual void step(double dt, const BrainSensoryInput& input, BrainMotorOutput& out) = 0;

    // Query neural topology
    [[nodiscard]] virtual size_t soma_count() const noexcept = 0;
    [[nodiscard]] virtual size_t active_neuron_count() const noexcept = 0;

    // Deterministic state hashing
    [[nodiscard]] virtual uint64_t compute_brain_hash() const noexcept = 0;

    // State serialization
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
};

} // namespace flgod::brain
