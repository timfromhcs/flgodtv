#include <cassert>
#include <iostream>
#include <memory>
#include "flgod/brain/fly_brain_interface.hpp"
#include "flgod/brain/malecns_adapter.hpp"

int main() {
    std::cout << "[Test] Running Fly Brain Interface Unit Tests...\n";

    // 1. Instantiation through abstract IFlyBrain pointer
    std::unique_ptr<flgod::brain::IFlyBrain> brain = std::make_unique<flgod::brain::MaleCNSAdapter>("", 1000);
    assert(brain->soma_count() == 1000);

    // 2. Baseline step with quiescent input
    flgod::brain::BrainSensoryInput quiescent_input;
    flgod::brain::BrainMotorOutput motor_out;
    brain->step(0.01, quiescent_input, motor_out);

    assert(motor_out.left_wing_freq_hz > 180.0 && motor_out.left_wing_freq_hz < 260.0);
    assert(motor_out.right_wing_freq_hz > 180.0 && motor_out.right_wing_freq_hz < 260.0);
    assert(motor_out.proboscis_extension == 0.0 && "PER should be 0 without sugar stimulus");

    // 3. Proboscis Extension Response (PER) to sugar olfactory/gustatory input
    flgod::brain::BrainSensoryInput sugar_input;
    sugar_input.odor_sugar_intensity = 0.9;
    flgod::brain::BrainMotorOutput per_out;
    brain->step(0.01, sugar_input, per_out);

    assert(per_out.proboscis_extension > 0.8 && "Proboscis should extend strongly when sugar is detected");

    // 4. Differential Wingbeat Asymmetrical Steering (Optomotor phototaxis)
    // Left eye stimulated heavily (1.0), right eye dark (0.0)
    flgod::brain::BrainSensoryInput left_bright_input;
    left_bright_input.left_eye_sectors.fill(1.0);
    left_bright_input.right_eye_sectors.fill(0.0);

    for (int i = 0; i < 5; ++i) {
        brain->step(0.01, left_bright_input, motor_out);
    }

    // Left visual drive activates left hemisphere, creating positive yaw steering torque
    assert(motor_out.steering_torque.y != 0.0);
    assert(motor_out.left_wing_freq_hz != motor_out.right_wing_freq_hz);

    // 5. Deterministic brain state hashing
    uint64_t h1 = brain->compute_brain_hash();
    assert(h1 != 0);
    (void)h1;

    std::cout << "[Test] Fly Brain Interface Unit Tests PASSED!\n";
    return 0;
}
