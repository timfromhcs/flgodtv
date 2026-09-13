#include "flgod/evolution/speciation.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_evolution_speciation..." << std::endl;

    flgod::SpeciationSystem spec_sys;

    flgod::Genome founder;
    founder.body.mass = 1.0;
    founder.brain.brain_volume = 1.0;

    // Closely related genome (same species)
    flgod::Genome similar = founder;
    similar.body.mass = 1.05;
    similar.brain.brain_volume = 1.02;

    // Highly divergent genome (different morphology and brain size)
    flgod::Genome divergent = founder;
    divergent.body.mass = 4.5;
    divergent.body.wing_span = 3.0;
    divergent.brain.brain_volume = 4.0;
    divergent.brain.neuron_count = 500000;
    divergent.regulatory_flags = 0x00000000;

    // 1. Check mating compatibility
    double compat_similar = spec_sys.calculate_mating_compatibility(founder, similar);
    double compat_divergent = spec_sys.calculate_mating_compatibility(founder, divergent);

    if (compat_similar < 0.9) {
        std::cerr << "FAILED: Similar genomes had low mating compatibility: " << compat_similar << std::endl;
        return 1;
    }
    if (compat_divergent > 0.1) {
        std::cerr << "FAILED: Highly divergent genomes did not experience reproductive isolation! Compat: " 
                  << compat_divergent << std::endl;
        return 1;
    }
    std::cout << "  Mating compatibility: similar=" << compat_similar 
              << ", divergent=" << compat_divergent << " (reproductive isolation confirmed)." << std::endl;

    // 2. Speciation detection
    uint32_t sp_sim = spec_sys.classify_or_speciate(similar, 1, 0);
    if (sp_sim != 0) {
        std::cerr << "FAILED: Similar genome incorrectly branched into new species!" << std::endl;
        return 1;
    }

    uint32_t sp_div = spec_sys.classify_or_speciate(divergent, 1, 0);
    if (sp_div == 0) {
        std::cerr << "FAILED: Divergent genome failed to trigger speciation event!" << std::endl;
        return 1;
    }
    std::cout << "  Speciation event successfully detected: new species ID " << sp_div 
              << " branched from ancestral species 0." << std::endl;

    if (spec_sys.species_count() < 2 || spec_sys.total_speciation_events() < 1) {
        std::cerr << "FAILED: Species count not updated properly!" << std::endl;
        return 1;
    }

    std::cout << "[TEST] test_evolution_speciation PASSED" << std::endl;
    return 0;
}
