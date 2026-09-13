#include "flgod/evolution/population.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_evolution_population..." << std::endl;

    auto setup_pop = []() {
        flgod::Population pop(30, 0.1);
        flgod::Genome founder;
        founder.body.mass = 1.0;
        founder.brain.neuron_count = 100000;
        pop.initialize_founders(founder);
        return pop;
    };

    // 1. Bit-exact determinism test across 2 independent populations
    flgod::Population pop1 = setup_pop();
    flgod::Population pop2 = setup_pop();

    flgod::RNGStream rng1(333999ULL);
    flgod::RNGStream rng2(333999ULL);

    const int total_generations = 25;
    for (int g = 0; g < total_generations; ++g) {
        pop1.step_generation(rng1);
        pop2.step_generation(rng2);

        uint64_t h1 = pop1.compute_hash();
        uint64_t h2 = pop2.compute_hash();

        if (h1 != h2) {
            std::cerr << "FAILED: Deterministic divergence at generation " << g
                      << ": h1=0x" << std::hex << h1 << " != h2=0x" << h2 << std::dec << std::endl;
            return 1;
        }
    }
    std::cout << "  Passed 25 generations bit-exact determinism check. Final hash: 0x" 
              << std::hex << pop1.compute_hash() << std::dec << std::endl;

    // 2. Checkpoint and Crash Recovery Test
    flgod::Population ref_pop = setup_pop();
    flgod::RNGStream ref_rng(333999ULL);
    for (int g = 0; g < total_generations; ++g) {
        ref_pop.step_generation(ref_rng);
    }
    uint64_t expected_hash = ref_pop.compute_hash();

    flgod::Population inter_pop = setup_pop();
    flgod::RNGStream inter_rng(333999ULL);
    for (int g = 0; g < 15; ++g) {
        inter_pop.step_generation(inter_rng);
    }

    // Save checkpoint
    nlohmann::json cp = inter_pop.to_json();
    std::string cp_str = cp.dump();

    // Restore into fresh population
    flgod::Population rec_pop;
    rec_pop.from_json(nlohmann::json::parse(cp_str));

    if (rec_pop.generation() != 15) {
        std::cerr << "FAILED: Restored population generation mismatch: " << rec_pop.generation() << " != 15" << std::endl;
        return 1;
    }

    // Continue to generation 25 with same continuing RNG state
    for (int g = 15; g < total_generations; ++g) {
        rec_pop.step_generation(inter_rng);
    }

    uint64_t recovered_hash = rec_pop.compute_hash();
    if (recovered_hash != expected_hash) {
        std::cerr << "FAILED: Restored population diverged! Expected: 0x" 
                  << std::hex << expected_hash << ", Got: 0x" << recovered_hash << std::dec << std::endl;
        return 1;
    }
    std::cout << "  Passed 100% crash recovery test at generation 15 -> 25. Hash: 0x" 
              << std::hex << recovered_hash << std::dec << std::endl;

    // Check migration tracking
    if (rec_pop.migration_history().empty()) {
        std::cerr << "FAILED: No migration events were tracked over 25 generations!" << std::endl;
        return 1;
    }
    std::cout << "  Recorded " << rec_pop.migration_history().size() 
              << " migration gene flow events across regional boundaries." << std::endl;

    std::cout << "[TEST] test_evolution_population PASSED" << std::endl;
    return 0;
}
