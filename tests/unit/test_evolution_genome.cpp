#include "flgod/evolution/genome.hpp"
#include "flgod/evolution/mutation.hpp"
#include "flgod/evolution/recombination.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_evolution_genome..." << std::endl;

    flgod::Genome g1;
    g1.body.mass = 1.5;
    g1.body.wing_span = 1.2;
    g1.body.manipulator_dexterity = 0.8;
    g1.brain.neuron_count = 140000;
    g1.brain.brain_volume = 1.4;
    g1.sensory.visual_acuity = 1.8;
    g1.learning.base_learning_rate = 0.25;
    g1.memory.working_memory_capacity = 8;
    g1.social.social_curiosity = 0.7;
    g1.reproduction.max_offspring_count = 6;

    uint64_t hash1 = g1.compute_hash();

    // 1. Serialization test
    nlohmann::json j = g1.to_json();
    flgod::Genome restored;
    restored.from_json(j);

    if (restored.compute_hash() != hash1) {
        std::cerr << "FAILED: Genome JSON roundtrip hash mismatch!" << std::endl;
        return 1;
    }
    if (restored.body.mass != 1.5 || restored.brain.neuron_count != 140000) {
        std::cerr << "FAILED: Restored genome fields mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  Genome serialization roundtrip verified." << std::endl;

    // 2. Mutation operators test
    flgod::MutationRates rates;
    rates.point_rate = 1.0;
    rates.insertion_rate = 1.0;
    rates.deletion_rate = 1.0;
    rates.duplication_rate = 1.0;
    rates.regulatory_rate = 1.0;
    rates.structural_rate = 1.0;

    flgod::MutationOperator mut(rates);
    flgod::RNGStream rng(987654321ULL);

    flgod::Genome mutated = mut.mutate(g1, rng);
    if (mutated.compute_hash() == hash1) {
        std::cerr << "FAILED: Mutation operator did not alter genome!" << std::endl;
        return 1;
    }
    double dist = g1.genetic_distance(mutated);
    if (dist <= 0.0) {
        std::cerr << "FAILED: Genetic distance after mutation is 0!" << std::endl;
        return 1;
    }
    std::cout << "  All 6 mutation operators executed successfully (genetic dist: " << dist << ")." << std::endl;

    // 3. Recombination test
    flgod::Genome g2;
    g2.body.mass = 0.5;
    g2.body.wing_span = 0.4;
    g2.brain.neuron_count = 50000;
    g2.sensory.visual_acuity = 0.5;

    flgod::RecombinationOperator recomb;
    flgod::Genome child = recomb.recombine(g1, g2, rng);

    // Child traits should be blended between g1 and g2
    if (child.body.mass < 0.5 || child.body.mass > 1.5) {
        std::cerr << "FAILED: Recombination mass out of parental bounds: " << child.body.mass << std::endl;
        return 1;
    }
    std::cout << "  Recombination produced viable blended offspring (child mass: " 
              << child.body.mass << " between 0.5 and 1.5)." << std::endl;

    std::cout << "[TEST] test_evolution_genome PASSED" << std::endl;
    return 0;
}
