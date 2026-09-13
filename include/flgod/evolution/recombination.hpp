#pragma once

#include "flgod/evolution/genome.hpp"
#include "flgod/core/rng.hpp"

namespace flgod {

enum class CrossoverMethod : uint8_t {
    ChromosomeAssortment = 0, // Independent chromosome assortment
    UniformBlending = 1,      // Homologous uniform gene crossover
    SinglePoint = 2           // Single-point sequence cut-and-splice
};

class RecombinationOperator {
public:
    explicit RecombinationOperator(CrossoverMethod method = CrossoverMethod::UniformBlending)
        : m_method(method) {}

    [[nodiscard]] Genome recombine(const Genome& parent_a, const Genome& parent_b, RNGStream& rng) const {
        Genome child;

        auto pick_bool = [&rng]() -> bool { return rng.next_double() < 0.5; };
        auto blend_double = [&rng](double a, double b) -> double {
            double alpha = rng.next_double();
            return a * alpha + b * (1.0 - alpha);
        };

        if (m_method == CrossoverMethod::ChromosomeAssortment) {
            // Independent assortment of gene blocks
            child.body = pick_bool() ? parent_a.body : parent_b.body;
            child.metabolism = pick_bool() ? parent_a.metabolism : parent_b.metabolism;
            child.sensory = pick_bool() ? parent_a.sensory : parent_b.sensory;
            child.brain = pick_bool() ? parent_a.brain : parent_b.brain;
            child.learning = pick_bool() ? parent_a.learning : parent_b.learning;
            child.memory = pick_bool() ? parent_a.memory : parent_b.memory;
            child.social = pick_bool() ? parent_a.social : parent_b.social;
            child.reproduction = pick_bool() ? parent_a.reproduction : parent_b.reproduction;
            child.regulatory_flags = pick_bool() ? parent_a.regulatory_flags : parent_b.regulatory_flags;

        } else if (m_method == CrossoverMethod::UniformBlending) {
            // Homologous gene-by-gene uniform crossover / blending
            child.body.mass = blend_double(parent_a.body.mass, parent_b.body.mass);
            child.body.wing_span = blend_double(parent_a.body.wing_span, parent_b.body.wing_span);
            child.body.manipulator_dexterity = blend_double(parent_a.body.manipulator_dexterity, parent_b.body.manipulator_dexterity);
            child.body.leg_count = pick_bool() ? parent_a.body.leg_count : parent_b.body.leg_count;

            child.metabolism.basal_metabolic_rate = blend_double(parent_a.metabolism.basal_metabolic_rate, parent_b.metabolism.basal_metabolic_rate);
            child.metabolism.energy_efficiency = blend_double(parent_a.metabolism.energy_efficiency, parent_b.metabolism.energy_efficiency);

            child.sensory.visual_acuity = blend_double(parent_a.sensory.visual_acuity, parent_b.sensory.visual_acuity);
            child.sensory.olfactory_sensitivity = blend_double(parent_a.sensory.olfactory_sensitivity, parent_b.sensory.olfactory_sensitivity);

            child.brain.brain_volume = blend_double(parent_a.brain.brain_volume, parent_b.brain.brain_volume);
            child.brain.neuron_count = static_cast<uint64_t>(blend_double(static_cast<double>(parent_a.brain.neuron_count), static_cast<double>(parent_b.brain.neuron_count)));
            child.brain.plasticity_rate = blend_double(parent_a.brain.plasticity_rate, parent_b.brain.plasticity_rate);

            child.learning.base_learning_rate = blend_double(parent_a.learning.base_learning_rate, parent_b.learning.base_learning_rate);
            child.learning.exploration_tendency = blend_double(parent_a.learning.exploration_tendency, parent_b.learning.exploration_tendency);

            child.memory.working_memory_capacity = pick_bool() ? parent_a.memory.working_memory_capacity : parent_b.memory.working_memory_capacity;
            child.memory.consolidation_efficiency = blend_double(parent_a.memory.consolidation_efficiency, parent_b.memory.consolidation_efficiency);

            child.social.social_curiosity = blend_double(parent_a.social.social_curiosity, parent_b.social.social_curiosity);
            child.social.imitation_tendency = blend_double(parent_a.social.imitation_tendency, parent_b.social.imitation_tendency);
            child.social.aggression_threshold = blend_double(parent_a.social.aggression_threshold, parent_b.social.aggression_threshold);

            child.reproduction.maturation_age_ticks = pick_bool() ? parent_a.reproduction.maturation_age_ticks : parent_b.reproduction.maturation_age_ticks;
            child.reproduction.max_offspring_count = pick_bool() ? parent_a.reproduction.max_offspring_count : parent_b.reproduction.max_offspring_count;

            uint32_t mask = static_cast<uint32_t>(rng.next_u64() & 0xFFFFFFFFULL);
            child.regulatory_flags = (parent_a.regulatory_flags & mask) | (parent_b.regulatory_flags & ~mask);

        } else { // SinglePoint
            child.body = parent_a.body;
            child.metabolism = parent_a.metabolism;
            child.sensory = parent_a.sensory;
            child.brain = parent_a.brain;
            child.learning = parent_b.learning;
            child.memory = parent_b.memory;
            child.social = parent_b.social;
            child.reproduction = parent_b.reproduction;
            child.regulatory_flags = parent_b.regulatory_flags;
        }

        return child;
    }

private:
    CrossoverMethod m_method;
};

} // namespace flgod
