#include "flgod/world/procedural_generator.hpp"
#include <iostream>
#include <unordered_set>

int main() {
    std::cout << "[TEST] Running test_world_procedural..." << std::endl;

    const uint64_t world_seed = 1029384756ULL;
    flgod::ProceduralGenerator gen1(world_seed);
    flgod::ProceduralGenerator gen2(world_seed);
    flgod::ProceduralGenerator gen3(world_seed + 1);

    // 1. Determinism test across multiple chunks
    for (int cz = -2; cz <= 2; ++cz) {
        for (int cx = -2; cx <= 2; ++cx) {
            flgod::ChunkCoord coord{.x = cx, .y = 0, .z = cz};
            flgod::WorldChunk c1 = gen1.generate_chunk(coord);
            flgod::WorldChunk c2 = gen2.generate_chunk(coord);

            if (c1.compute_hash() != c2.compute_hash()) {
                std::cerr << "FAILED: Deterministic procedural generation hash mismatch at "
                          << coord.to_string() << std::endl;
                return 1;
            }

            // Verify height values are bit-exact
            for (uint32_t lz = 0; lz < flgod::CHUNK_SIZE; ++lz) {
                for (uint32_t lx = 0; lx < flgod::CHUNK_SIZE; ++lx) {
                    if (c1.get_height(lx, lz) != c2.get_height(lx, lz)) {
                        std::cerr << "FAILED: Height value mismatch at (" << lx << ", " << lz << ")" << std::endl;
                        return 1;
                    }
                    if (c1.get_biome(lx, lz) != c2.get_biome(lx, lz)) {
                        std::cerr << "FAILED: Biome mismatch at (" << lx << ", " << lz << ")" << std::endl;
                        return 1;
                    }
                }
            }
        }
    }

    // 2. Different world seeds produce different chunks
    flgod::ChunkCoord center_coord{0, 0, 0};
    flgod::WorldChunk c_seed1 = gen1.generate_chunk(center_coord);
    flgod::WorldChunk c_seed3 = gen3.generate_chunk(center_coord);

    if (c_seed1.compute_hash() == c_seed3.compute_hash()) {
        std::cerr << "FAILED: Different world seeds produced identical chunk hash!" << std::endl;
        return 1;
    }

    // 3. Biome variety check across a wide area (10x10 chunks)
    std::unordered_set<flgod::BiomeType> observed_biomes;
    for (int cz = -5; cz <= 5; ++cz) {
        for (int cx = -5; cx <= 5; ++cx) {
            flgod::WorldChunk c = gen1.generate_chunk(flgod::ChunkCoord{cx, 0, cz});
            for (uint32_t lz = 0; lz < flgod::CHUNK_SIZE; lz += 4) {
                for (uint32_t lx = 0; lx < flgod::CHUNK_SIZE; lx += 4) {
                    observed_biomes.insert(c.get_biome(lx, lz));
                }
            }
        }
    }

    std::cout << "[INFO] Observed " << observed_biomes.size() << " distinct biomes across region." << std::endl;
    if (observed_biomes.size() < 3) {
        std::cerr << "FAILED: Insufficient biome diversity: only " << observed_biomes.size() << " biomes observed!" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_world_procedural passed successfully." << std::endl;
    return 0;
}
