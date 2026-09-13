#include "flgod/evolution/population.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_evolution_coevolution..." << std::endl;

    flgod::Individual small_brain;
    small_brain.genome.body.mass = 0.5;
    small_brain.genome.brain.neuron_count = 50000;
    small_brain.genome.brain.metabolic_cost_per_neuron = 1e-5;
    small_brain.genome.body.manipulator_dexterity = 0.2;

    flgod::Individual large_brain_poor_body;
    large_brain_poor_body.genome.body.mass = 0.5;
    large_brain_poor_body.genome.brain.neuron_count = 250000;
    large_brain_poor_body.genome.brain.metabolic_cost_per_neuron = 1e-5;
    large_brain_poor_body.genome.body.manipulator_dexterity = 0.2; // cannot forage efficiently

    flgod::Individual large_brain_good_body;
    large_brain_good_body.genome.body.mass = 1.0;
    large_brain_good_body.genome.brain.neuron_count = 250000;
    large_brain_good_body.genome.brain.metabolic_cost_per_neuron = 1e-5;
    large_brain_good_body.genome.body.manipulator_dexterity = 0.9; // highly dexterous
    large_brain_good_body.genome.sensory.visual_acuity = 2.0;

    double cost_small = small_brain.calculate_metabolic_cost();
    double cost_large_poor = large_brain_poor_body.calculate_metabolic_cost();

    if (cost_large_poor <= cost_small) {
        std::cerr << "FAILED: Large brain did not incur higher metabolic cost! cost_large=" 
                  << cost_large_poor << ", cost_small=" << cost_small << std::endl;
        return 1;
    }
    std::cout << "  Body/brain metabolic cost constraint verified: small=" 
              << cost_small << " W, large=" << cost_large_poor << " W." << std::endl;

    // Simulate resource foraging over 100 ticks
    flgod::RNGStream rng(555123ULL);
    for (int t = 0; t < 100; ++t) {
        // Poor body forager
        if (rng.next_double() < small_brain.genome.body.manipulator_dexterity * 0.5) {
            small_brain.resources_foraged++;
            small_brain.energy_consumed += 15.0;
        }
        small_brain.energy_expended += cost_small;

        if (rng.next_double() < large_brain_poor_body.genome.body.manipulator_dexterity * 0.5) {
            large_brain_poor_body.resources_foraged++;
            large_brain_poor_body.energy_consumed += 15.0;
        }
        large_brain_poor_body.energy_expended += cost_large_poor;

        // Good body forager
        if (rng.next_double() < large_brain_good_body.genome.body.manipulator_dexterity * 0.6) {
            large_brain_good_body.resources_foraged += 2;
            large_brain_good_body.energy_consumed += 30.0;
        }
        large_brain_good_body.energy_expended += large_brain_good_body.calculate_metabolic_cost();
    }

    double fit_small = small_brain.calculate_fitness();
    double fit_large_poor = large_brain_poor_body.calculate_fitness();
    double fit_large_good = large_brain_good_body.calculate_fitness();

    std::cout << "  Simulated fitness outcomes:\n"
              << "    Small brain: " << fit_small << "\n"
              << "    Large brain (poor body): " << fit_large_poor << "\n"
              << "    Large brain (adapted body): " << fit_large_good << "\n";

    // Large brain without morphology to sustain it has lower fitness than small brain!
    if (fit_large_poor >= fit_small) {
        std::cerr << "FAILED: Unadapted large brain did not suffer energetic deficit!" << std::endl;
        return 1;
    }
    // Large brain with adapted morphology achieves highest fitness
    if (fit_large_good <= fit_small) {
        std::cerr << "FAILED: Co-adapted large brain did not outperform small brain!" << std::endl;
        return 1;
    }

    std::cout << "  Body/brain co-evolution dynamics validated: metabolic penalty prevents unviable brain growth without supporting morphology." << std::endl;

    std::cout << "[TEST] test_evolution_coevolution PASSED" << std::endl;
    return 0;
}
