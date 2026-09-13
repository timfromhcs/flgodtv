#pragma once

#include "flgod/evolution/genome.hpp"
#include "flgod/evolution/mutation.hpp"
#include "flgod/evolution/recombination.hpp"
#include "flgod/evolution/speciation.hpp"
#include "flgod/core/rng.hpp"
#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

struct MigrationEvent {
    uint32_t origin_region{0};
    uint32_t destination_region{1};
    uint64_t individual_id{0};
    uint32_t species_id{0};
    double genetic_divergence_from_dest{0.0};
    uint32_t generation{0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"origin", origin_region},
            {"dest", destination_region},
            {"id", individual_id},
            {"species", species_id},
            {"divergence", genetic_divergence_from_dest},
            {"gen", generation}
        };
    }
    void from_json(const nlohmann::json& j) {
        origin_region = j.value("origin", 0u);
        destination_region = j.value("dest", 1u);
        individual_id = j.value("id", 0ULL);
        species_id = j.value("species", 0u);
        genetic_divergence_from_dest = j.value("divergence", 0.0);
        generation = j.value("gen", 0u);
    }
};

struct Individual {
    uint64_t id{0};
    uint32_t species_id{0};
    uint32_t region_id{0};
    Genome genome{};
    double energy{100.0};
    uint32_t age_ticks{0};
    bool is_alive{true};

    // Real world outcome signals (GEMINI.md Section 43)
    uint32_t survival_ticks{0};
    double energy_consumed{0.0};
    double energy_expended{0.0};
    uint32_t resources_foraged{0};
    uint32_t offspring_produced{0};
    uint32_t social_cooperations{0};

    // Body/brain co-evolution constraint (GEMINI.md Section 49)
    [[nodiscard]] double calculate_metabolic_cost() const noexcept {
        double basal = genome.metabolism.basal_metabolic_rate;
        double body_cost = 0.2 * genome.body.mass;
        double brain_cost = genome.brain.metabolic_cost_per_neuron * static_cast<double>(genome.brain.neuron_count);
        return basal + body_cost + brain_cost;
    }

    // Fitness calculated purely from real world outcomes (GEMINI.md Section 43)
    [[nodiscard]] double calculate_fitness() const noexcept {
        double net_energy = std::max(0.0, energy_consumed - energy_expended);
        double f = (survival_ticks * 0.05) +
                   (net_energy * 0.2) +
                   (resources_foraged * 1.5) +
                   (offspring_produced * 4.0) +
                   (social_cooperations * 0.5);
        return std::max(0.01, f);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", id}, {"species", species_id}, {"region", region_id},
            {"energy", energy}, {"age", age_ticks}, {"alive", is_alive},
            {"survival", survival_ticks}, {"consumed", energy_consumed},
            {"expended", energy_expended}, {"foraged", resources_foraged},
            {"offspring", offspring_produced}, {"social", social_cooperations},
            {"genome", genome.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        id = j.value("id", 0ULL);
        species_id = j.value("species", 0u);
        region_id = j.value("region", 0u);
        energy = j.value("energy", 100.0);
        age_ticks = j.value("age", 0u);
        is_alive = j.value("alive", true);
        survival_ticks = j.value("survival", 0u);
        energy_consumed = j.value("consumed", 0.0);
        energy_expended = j.value("expended", 0.0);
        resources_foraged = j.value("foraged", 0u);
        offspring_produced = j.value("offspring", 0u);
        social_cooperations = j.value("social", 0u);
        if (j.contains("genome")) genome.from_json(j["genome"]);
    }
};

class Population {
public:
    explicit Population(size_t target_size = 50, double drift_strength = 0.15)
        : m_target_size(target_size), m_drift_strength(drift_strength) {}

    void initialize_founders(const Genome& founder_genome = Genome{}) {
        m_individuals.clear();
        m_individuals.reserve(m_target_size);
        for (size_t i = 0; i < m_target_size; ++i) {
            Individual ind;
            ind.id = m_next_individual_id++;
            ind.species_id = 0;
            ind.region_id = static_cast<uint32_t>(i % 2); // 2 founding regions
            ind.genome = founder_genome;
            ind.energy = 100.0;
            m_individuals.push_back(ind);
        }
        m_generation = 0;
    }

    [[nodiscard]] size_t size() const noexcept { return m_individuals.size(); }
    [[nodiscard]] uint32_t generation() const noexcept { return m_generation; }
    [[nodiscard]] const std::vector<Individual>& individuals() const noexcept { return m_individuals; }
    [[nodiscard]] std::vector<Individual>& individuals() noexcept { return m_individuals; }
    [[nodiscard]] const SpeciationSystem& speciation() const noexcept { return m_speciation; }
    [[nodiscard]] const std::vector<MigrationEvent>& migration_history() const noexcept { return m_migrations; }

    // Step generation: selection, recombination, mutation, drift, migration, speciation
    void step_generation(RNGStream& rng) {
        if (m_individuals.empty()) return;

        // 1. Simulate metabolic cost and real world outcome accumulation
        for (auto& ind : m_individuals) {
            double cost = ind.calculate_metabolic_cost();
            ind.energy_expended += cost * 10.0;
            // Better dexterity and visual acuity -> higher resource forage probability
            double forage_chance = ind.genome.body.manipulator_dexterity * 0.6 + ind.genome.sensory.visual_acuity * 0.4;
            if (rng.next_double() < forage_chance) {
                ind.resources_foraged += static_cast<uint32_t>(1 + rng.next_u64() % 3);
                ind.energy_consumed += 25.0 * ind.genome.metabolism.energy_efficiency;
            }
            ind.survival_ticks += 60;
            ind.age_ticks += 60;
        }

        // 2. Stochastic Selection with Genetic Drift (GEMINI.md Section 44)
        // We use Boltzmann / softmax tournament selection where drift_strength allows neutral & non-adaptive survival
        std::vector<double> fitnesses(m_individuals.size());
        for (size_t i = 0; i < m_individuals.size(); ++i) {
            fitnesses[i] = m_individuals[i].calculate_fitness();
        }

        auto select_parent = [this, &fitnesses, &rng]() -> const Individual& {
            // Tournament selection with stochastic drift noise
            size_t idx1 = static_cast<size_t>(rng.next_u64() % m_individuals.size());
            size_t idx2 = static_cast<size_t>(rng.next_u64() % m_individuals.size());

            double noisy_f1 = fitnesses[idx1] * (1.0 + rng.next_gaussian() * m_drift_strength);
            double noisy_f2 = fitnesses[idx2] * (1.0 + rng.next_gaussian() * m_drift_strength);

            return (noisy_f1 >= noisy_f2) ? m_individuals[idx1] : m_individuals[idx2];
        };

        // 3. Create next generation via recombination & mutation
        std::vector<Individual> next_generation;
        next_generation.reserve(m_target_size);

        while (next_generation.size() < m_target_size) {
            const auto& parent_a = select_parent();
            const auto& parent_b = select_parent();

            // Check mating compatibility (GEMINI.md Section 47)
            if (!m_speciation.can_interbreed(parent_a.genome, parent_b.genome)) {
                // Pre-zygotic isolation: mating fails, try another pair
                continue;
            }

            // Recombination (GEMINI.md Section 45)
            Genome child_genome = m_recombination.recombine(parent_a.genome, parent_b.genome, rng);

            // Offspring viability check (post-zygotic isolation)
            double viability = m_speciation.calculate_offspring_viability(parent_a.genome, parent_b.genome);
            if (rng.next_double() > viability) {
                // Non-viable offspring due to genetic divergence!
                continue;
            }

            // Mutation (GEMINI.md Section 42)
            child_genome = m_mutation.mutate(child_genome, rng);

            // Speciation detection (GEMINI.md Section 47)
            uint32_t species = m_speciation.classify_or_speciate(child_genome, m_generation + 1, parent_a.species_id);

            Individual child;
            child.id = m_next_individual_id++;
            child.species_id = species;
            child.region_id = parent_a.region_id; // inherit maternal region
            child.genome = child_genome;
            child.energy = 100.0;
            next_generation.push_back(child);
        }

        // 4. Migration & Gene Flow (GEMINI.md Section 46)
        double migration_rate = 0.05; // 5% chance to migrate to neighbor region
        for (auto& ind : next_generation) {
            if (rng.next_double() < migration_rate) {
                uint32_t origin = ind.region_id;
                uint32_t dest = 1 - origin; // flip region (0 <-> 1)
                ind.region_id = dest;

                MigrationEvent event;
                event.origin_region = origin;
                event.destination_region = dest;
                event.individual_id = ind.id;
                event.species_id = ind.species_id;
                event.generation = m_generation + 1;
                event.genetic_divergence_from_dest = m_speciation.calculate_genetic_divergence(ind.genome, m_speciation.all_species().at(ind.species_id).prototype_genome);
                m_migrations.push_back(event);
            }
        }

        m_individuals = std::move(next_generation);
        m_generation++;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        h ^= m_generation;
        h *= 1099511628211ULL;
        for (const auto& ind : m_individuals) {
            h ^= ind.genome.compute_hash();
            h ^= ind.species_id;
            h ^= ind.region_id;
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json ind_arr = nlohmann::json::array();
        for (const auto& ind : m_individuals) ind_arr.push_back(ind.to_json());
        nlohmann::json mig_arr = nlohmann::json::array();
        for (const auto& mig : m_migrations) mig_arr.push_back(mig.to_json());

        return {
            {"generation", m_generation},
            {"target_size", m_target_size},
            {"drift_strength", m_drift_strength},
            {"speciation", m_speciation.to_json()},
            {"individuals", ind_arr},
            {"migrations", mig_arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_generation = j.value("generation", 0u);
        m_target_size = j.value("target_size", 50ULL);
        m_drift_strength = j.value("drift_strength", 0.15);
        if (j.contains("speciation")) m_speciation.from_json(j["speciation"]);
        m_individuals.clear();
        if (j.contains("individuals")) {
            for (const auto& item : j["individuals"]) {
                Individual ind;
                ind.from_json(item);
                m_individuals.push_back(ind);
            }
        }
        m_migrations.clear();
        if (j.contains("migrations")) {
            for (const auto& item : j["migrations"]) {
                MigrationEvent m;
                m.from_json(item);
                m_migrations.push_back(m);
            }
        }
    }

private:
    size_t m_target_size{50};
    double m_drift_strength{0.15};
    uint32_t m_generation{0};
    uint64_t m_next_individual_id{1};
    std::vector<Individual> m_individuals;
    std::vector<MigrationEvent> m_migrations;
    SpeciationSystem m_speciation;
    MutationOperator m_mutation;
    RecombinationOperator m_recombination;
};

} // namespace flgod
