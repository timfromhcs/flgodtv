#include "flgod/world/world.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_deterministic_world..." << std::endl;

    flgod::WorldConfig configA{
        .world_seed = 888101ULL,
        .weather_seed = 888102ULL,
        .fields_width = 32,
        .fields_height = 32,
        .fields_cell_size = 2.0
    };

    flgod::WorldConfig configB = configA;

    // 1. Reference Run A: Run uninterrupted for 200 ticks
    flgod::World worldA(configA);

    // Populate a 3x3 chunk region around (0,0)
    for (int cz = -1; cz <= 1; ++cz) {
        for (int cx = -1; cx <= 1; ++cx) {
            worldA.get_or_create_chunk(flgod::ChunkCoord{cx, 0, cz});
        }
    }

    // Apply some player/agent modifications to chunk (0,0)
    worldA.get_or_create_chunk(flgod::ChunkCoord{0, 0, 0}).apply_height_delta(4, 4, 1.25);

    nlohmann::json checkpoint_at_100;
    uint64_t hash_at_100_A = 0;

    for (int t = 0; t < 200; ++t) {
        worldA.step(1.0 / 60.0, t * (1.0 / 60.0));
        if (t == 99) { // At tick 100
            checkpoint_at_100 = worldA.to_json();
            hash_at_100_A = worldA.compute_world_hash();
        }
    }
    uint64_t final_hash_A = worldA.compute_world_hash();

    // 2. Parallel Run B: Run uninterrupted with identical config
    flgod::World worldB(configB);
    for (int cz = -1; cz <= 1; ++cz) {
        for (int cx = -1; cx <= 1; ++cx) {
            worldB.get_or_create_chunk(flgod::ChunkCoord{cx, 0, cz});
        }
    }
    worldB.get_or_create_chunk(flgod::ChunkCoord{0, 0, 0}).apply_height_delta(4, 4, 1.25);

    for (int t = 0; t < 200; ++t) {
        worldB.step(1.0 / 60.0, t * (1.0 / 60.0));
    }
    uint64_t final_hash_B = worldB.compute_world_hash();

    if (final_hash_A != final_hash_B) {
        std::cerr << "FAILED: Deterministic divergence in World! Final hash A (" << final_hash_A
                  << ") != Final hash B (" << final_hash_B << ")" << std::endl;
        return 1;
    }
    std::cout << "[CHECK 1/2] Parallel identical world runs matched final hash: " << final_hash_A << std::endl;

    // 3. Crash Recovery Run C: Restore from tick 100 checkpoint and continue to tick 200
    flgod::World worldC(configA);
    worldC.from_json(checkpoint_at_100);

    // Re-populate the same chunks
    for (int cz = -1; cz <= 1; ++cz) {
        for (int cx = -1; cx <= 1; ++cx) {
            worldC.get_or_create_chunk(flgod::ChunkCoord{cx, 0, cz});
        }
    }

    uint64_t restored_hash_C = worldC.compute_world_hash();
    if (restored_hash_C != hash_at_100_A) {
        std::cerr << "FAILED: Restored world hash at tick 100 (" << restored_hash_C
                  << ") != Reference hash at tick 100 (" << hash_at_100_A << ")" << std::endl;
        return 1;
    }

    // Step from tick 100 to tick 200
    for (int t = 100; t < 200; ++t) {
        worldC.step(1.0 / 60.0, t * (1.0 / 60.0));
    }
    uint64_t final_hash_C = worldC.compute_world_hash();

    if (final_hash_C != final_hash_A) {
        std::cerr << "FAILED: Restored run divergence! Final hash C (" << final_hash_C
                  << ") != Reference hash A (" << final_hash_A << ")" << std::endl;
        return 1;
    }
    std::cout << "[CHECK 2/2] Restored checkpoint run matched reference final hash: " << final_hash_C << std::endl;

    std::cout << "[PASS] test_deterministic_world passed successfully." << std::endl;
    return 0;
}
