#pragma once

#include "flgod/evolution/genome.hpp"
#include "flgod/core/rng.hpp"

namespace flgod {

struct MutationRates {
    double point_rate{0.15};
    double insertion_rate{0.05};
    double deletion_rate{0.05};
    double duplication_rate{0.03};
    double regulatory_rate{0.05};
    double structural_rate{0.02};
    double perturb_magnitude{0.1}; // standard deviation for Gaussian point mutations
};

class MutationOperator {
public:
    explicit MutationOperator(const MutationRates& rates = MutationRates{})
        : m_rates(rates) {}

    [[nodiscard]] Genome mutate(const Genome& parent, RNGStream& rng) const {
        Genome child = parent;

        // 1. Point mutations: Gaussian perturbations to continuous traits
        if (rng.next_double() < m_rates.point_rate) {
            mutate_point(child, rng);
        }

        // 2. Insertion mutation: Increases morphological / cognitive capacity
        if (rng.next_double() < m_rates.insertion_rate) {
            mutate_insertion(child, rng);
        }

        // 3. Deletion mutation: Decreases non-vital gene expression or capacity
        if (rng.next_double() < m_rates.deletion_rate) {
            mutate_deletion(child, rng);
        }

        // 4. Duplication mutation: Tandem duplication of functional structures
        if (rng.next_double() < m_rates.duplication_rate) {
            mutate_duplication(child, rng);
        }

        // 5. Regulatory mutation: Toggles gene expression regulatory bits
        if (rng.next_double() < m_rates.regulatory_rate) {
            mutate_regulatory(child, rng);
        }

        // 6. Structural mutation: Inverts or redistributes internal regional allocations
        if (rng.next_double() < m_rates.structural_rate) {
            mutate_structural(child, rng);
        }

        return child;
    }

    void mutate_point(Genome& g, RNGStream& rng) const {
        auto perturb = [this, &rng](double val, double min_v, double max_v) -> double {
            double delta = rng.next_gaussian() * m_rates.perturb_magnitude;
            return std::clamp(val + delta, min_v, max_v);
        };

        g.body.mass = perturb(g.body.mass, 0.1, 10.0);
        g.body.wing_span = perturb(g.body.wing_span, 0.1, 5.0);
        g.body.manipulator_dexterity = perturb(g.body.manipulator_dexterity, 0.0, 1.0);
        g.metabolism.basal_metabolic_rate = perturb(g.metabolism.basal_metabolic_rate, 0.1, 10.0);
        g.sensory.visual_acuity = perturb(g.sensory.visual_acuity, 0.1, 5.0);
        g.sensory.olfactory_sensitivity = perturb(g.sensory.olfactory_sensitivity, 0.1, 5.0);
        g.brain.brain_volume = perturb(g.brain.brain_volume, 0.2, 5.0);
        g.brain.plasticity_rate = perturb(g.brain.plasticity_rate, 0.001, 0.5);
        g.learning.base_learning_rate = perturb(g.learning.base_learning_rate, 0.01, 1.0);
        g.social.social_curiosity = perturb(g.social.social_curiosity, 0.0, 1.0);
    }

    void mutate_insertion(Genome& g, RNGStream& rng) const {
        // Insert working memory slot or sensory channels
        if (rng.next_double() < 0.5) {
            if (g.memory.working_memory_capacity < 15) {
                g.memory.working_memory_capacity++;
            }
        } else {
            // Increase neuron count
            g.brain.neuron_count += static_cast<uint64_t>(rng.next_double() * 5000.0);
        }
    }

    void mutate_deletion(Genome& g, RNGStream& rng) const {
        if (rng.next_double() < 0.5) {
            if (g.memory.working_memory_capacity > 3) {
                g.memory.working_memory_capacity--;
            }
        } else {
            if (g.brain.neuron_count > 10000) {
                g.brain.neuron_count -= std::min(g.brain.neuron_count - 10000, static_cast<uint64_t>(rng.next_double() * 3000.0));
            }
        }
    }

    void mutate_duplication(Genome& g, RNGStream& /*rng*/) const {
        // Duplicate sensory allocation or fecundity
        g.reproduction.max_offspring_count = std::min(12u, g.reproduction.max_offspring_count + 1);
        g.sensory.olfactory_sensitivity *= 1.2;
    }

    void mutate_regulatory(Genome& g, RNGStream& rng) const {
        uint32_t bit_pos = static_cast<uint32_t>(rng.next_u64() % 32);
        g.regulatory_flags ^= (1u << bit_pos); // flip regulatory bit
    }

    void mutate_structural(Genome& g, RNGStream& rng) const {
        // Swap or re-weight sensory vs association brain allocations
        if (rng.next_double() < 0.5) {
            std::swap(g.brain.sensory_region_allocation, g.brain.association_region_allocation);
        } else {
            std::swap(g.brain.motor_region_allocation, g.brain.association_region_allocation);
        }
    }

private:
    MutationRates m_rates;
};

} // namespace flgod
