#include "flgod/world/chunk.hpp"
#include <iostream>
#include <cassert>

int main() {
    try {
        std::cout << "[TEST] Running test_world_chunk..." << std::endl;

        // 1. Coordinate packing and unpacking test across negative and positive ranges
        flgod::ChunkCoord coords_to_test[] = {
            {0, 0, 0},
            {10, 0, -25},
            {-500, 12, 1000},
            {-100000, 500, -100000},
            {100000, -500, 100000}
        };

        for (const auto& original : coords_to_test) {
            flgod::ChunkID id = flgod::coord_to_chunk_id(original);
            flgod::ChunkCoord restored = flgod::chunk_id_to_coord(id);
            if (restored.x != original.x || restored.y != original.y || restored.z != original.z) {
                std::cerr << "FAILED: Coordinate packing mismatch for " << original.to_string()
                          << " -> got " << restored.to_string() << std::endl;
                return 1;
            }
        }
        std::cout << "[STEP 1 PASS]" << std::endl;

        // 2. Chunk creation & data manipulation
        flgod::ChunkCoord coord{3, 0, -7};
        uint64_t seed = flgod::compute_chunk_seed(424242ULL, coord, 1);
        flgod::WorldChunk chunk(coord, seed);

        chunk.set_base_height(5, 5, 18.5);
        chunk.set_base_biome(5, 5, flgod::BiomeType::Forest);
        chunk.set_base_moisture(5, 5, 0.75);
        chunk.set_base_water_height(5, 5, 2.0);

        if (chunk.get_height(5, 5) != 18.5) {
            std::cerr << "FAILED: Base height incorrect" << std::endl;
            return 1;
        }
        if (chunk.get_biome(5, 5) != flgod::BiomeType::Forest) {
            std::cerr << "FAILED: Base biome incorrect" << std::endl;
            return 1;
        }
        std::cout << "[STEP 2 PASS]" << std::endl;

        // 3. Dynamic delta application
        if (!chunk.delta().empty()) {
            std::cerr << "FAILED: Initial chunk delta should be empty" << std::endl;
            return 1;
        }

        chunk.apply_height_delta(5, 5, -3.5); // dug down 3.5m
        chunk.set_biome_override(5, 5, flgod::BiomeType::Desert); // scorched/desertified

        if (chunk.delta().empty()) {
            std::cerr << "FAILED: Chunk delta should not be empty after edit" << std::endl;
            return 1;
        }
        if (chunk.get_height(5, 5) != 15.0) { // 18.5 - 3.5 = 15.0
            std::cerr << "FAILED: Modified height should be 15.0, got " << chunk.get_height(5, 5) << std::endl;
            return 1;
        }
        if (chunk.get_biome(5, 5) != flgod::BiomeType::Desert) {
            std::cerr << "FAILED: Modified biome should be Desert" << std::endl;
            return 1;
        }
        std::cout << "[STEP 3 PASS]" << std::endl;

        // 4. Delta serialization & restore
        nlohmann::json chunk_json = chunk.to_json();
        std::cout << "[JSON]: " << chunk_json.dump() << std::endl;
        flgod::WorldChunk restored_chunk(coord, seed);
        restored_chunk.set_base_height(5, 5, 18.5);
        restored_chunk.set_base_biome(5, 5, flgod::BiomeType::Forest);
        restored_chunk.from_json(chunk_json);

        if (restored_chunk.get_height(5, 5) != 15.0) {
            std::cerr << "FAILED: Restored chunk modified height incorrect: " << restored_chunk.get_height(5, 5) << std::endl;
            return 1;
        }
        if (restored_chunk.get_biome(5, 5) != flgod::BiomeType::Desert) {
            std::cerr << "FAILED: Restored chunk modified biome incorrect" << std::endl;
            return 1;
        }
        std::cout << "[STEP 4 PASS]" << std::endl;

        std::cout << "[PASS] test_world_chunk passed successfully." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "EXCEPTION in test_world_chunk: " << ex.what() << std::endl;
        return 1;
    }
}
