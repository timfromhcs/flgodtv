#include "flgod/core/clock.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_core_clock..." << std::endl;

    const double fixed_dt = 1.0 / 60.0;
    flgod::SimulationClock clock(fixed_dt);

    // Initial state check
    if (clock.tick() != 0) {
        std::cerr << "FAILED: Initial tick should be 0, got " << clock.tick() << std::endl;
        return 1;
    }
    if (clock.elapsed_time() != 0.0) {
        std::cerr << "FAILED: Initial elapsed_time should be 0.0, got " << clock.elapsed_time() << std::endl;
        return 1;
    }

    uint64_t initial_hash = clock.compute_hash();

    // Step 120 times (2.0 simulation seconds)
    for (int i = 0; i < 120; ++i) {
        flgod::SimulationStep step = clock.step();
        if (step.tick != static_cast<uint64_t>(i)) {
            std::cerr << "FAILED: Expected step.tick " << i << ", got " << step.tick << std::endl;
            return 1;
        }
        if (std::abs(step.dt - fixed_dt) > 1e-9) {
            std::cerr << "FAILED: Expected step.dt " << fixed_dt << ", got " << step.dt << std::endl;
            return 1;
        }
    }

    if (clock.tick() != 120) {
        std::cerr << "FAILED: Final tick should be 120, got " << clock.tick() << std::endl;
        return 1;
    }

    double expected_time = 120.0 * fixed_dt;
    if (std::abs(clock.elapsed_time() - expected_time) > 1e-6) {
        std::cerr << "FAILED: Expected elapsed time " << expected_time << ", got " << clock.elapsed_time() << std::endl;
        return 1;
    }

    uint64_t step120_hash = clock.compute_hash();
    if (step120_hash == initial_hash) {
        std::cerr << "FAILED: Clock hash must change when state changes!" << std::endl;
        return 1;
    }

    // Reset check
    clock.reset(0, 0.0);
    if (clock.compute_hash() != initial_hash) {
        std::cerr << "FAILED: Reset clock hash does not match initial hash!" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_core_clock passed successfully." << std::endl;
    return 0;
}
