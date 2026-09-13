#include "flgod/core/simulation.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "[TEST] Running test_deterministic_simulation..." << std::endl;

    flgod::SimulationConfig configA{
        .version = flgod::CURRENT_SIMULATION_VERSION,
        .seeds = {
            .world_seed = 99901,
            .weather_seed = 99902,
            .physics_seed = 99903,
            .agent_seed = 99904,
            .genome_seed = 99905,
            .event_seed = 99906,
            .learning_seed = 99907,
            .render_seed = 99908
        },
        .fixed_dt = 1.0 / 60.0,
        .start_tick = 0,
        .start_time = 0.0,
        .experiment_id = "determinism_audit_01"
    };

    flgod::SimulationConfig configB = configA; // Identical configuration

    // 1. Reference Run A: Run uninterrupted for 300 ticks
    flgod::Simulation simA;
    simA.initialize(configA);

    // Track state at midpoint (tick 150) for checkpoint test
    nlohmann::json checkpoint_at_150;
    uint64_t hash_at_150_A = 0;

    for (int t = 0; t < 300; ++t) {
        // Deterministically mutate state via RNG consumption
        double r = simA.state().rng().agent().next_double();
        if (r > 0.5) {
            simA.state().id_allocator().allocate(flgod::EntityType::Agent);
        }
        simA.step();

        if (t == 149) { // At end of tick 150
            checkpoint_at_150 = simA.create_checkpoint();
            hash_at_150_A = simA.compute_state_hash();
        }
    }
    uint64_t final_hash_A = simA.compute_state_hash();

    // 2. Parallel Run B: Run uninterrupted with identical seed
    flgod::Simulation simB;
    simB.initialize(configB);

    for (int t = 0; t < 300; ++t) {
        double r = simB.state().rng().agent().next_double();
        if (r > 0.5) {
            simB.state().id_allocator().allocate(flgod::EntityType::Agent);
        }
        simB.step();
    }
    uint64_t final_hash_B = simB.compute_state_hash();

    if (final_hash_A != final_hash_B) {
        std::cerr << "FAILED: Deterministic divergence! Final hash A (" << final_hash_A
                  << ") != Final hash B (" << final_hash_B << ")" << std::endl;
        return 1;
    }
    std::cout << "[CHECK 1/2] Uninterrupted parallel runs produced identical hash: " << final_hash_A << std::endl;

    // 3. Crash Recovery Run C: Restore from tick 150 checkpoint and continue to tick 300
    flgod::Simulation simC;
    simC.initialize(configA); // start fresh
    simC.restore_checkpoint(checkpoint_at_150);

    uint64_t restored_hash_C = simC.compute_state_hash();
    if (restored_hash_C != hash_at_150_A) {
        std::cerr << "FAILED: Restored hash at tick 150 (" << restored_hash_C
                  << ") != Original hash at tick 150 (" << hash_at_150_A << ")" << std::endl;
        return 1;
    }

    // Continue from tick 150 to tick 300
    for (int t = 150; t < 300; ++t) {
        double r = simC.state().rng().agent().next_double();
        if (r > 0.5) {
            simC.state().id_allocator().allocate(flgod::EntityType::Agent);
        }
        simC.step();
    }
    uint64_t final_hash_C = simC.compute_state_hash();

    if (final_hash_C != final_hash_A) {
        std::cerr << "FAILED: Crash recovery divergence! Restored run final hash C (" << final_hash_C
                  << ") != Reference run final hash A (" << final_hash_A << ")" << std::endl;
        return 1;
    }
    std::cout << "[CHECK 2/2] Restored checkpoint run matched reference final hash: " << final_hash_C << std::endl;

    std::cout << "[PASS] test_deterministic_simulation passed successfully." << std::endl;
    return 0;
}
