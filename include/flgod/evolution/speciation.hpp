#pragma once

#include "flgod/evolution/genome.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <nlohmann/json.hpp>

namespace flgod {

struct SpeciesInfo {
    uint32_t species_id{0};
    uint32_t ancestral_species_id{0};
    uint32_t generation_formed{0};
    uint32_t member_count{0};
    Genome prototype_genome{};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"id", species_id},
            {"ancestor_id", ancestral_species_id},
            {"gen_formed", generation_formed},
            {"members", member_count},
            {"prototype", prototype_genome.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        species_id = j.value("id", 0u);
        ancestral_species_id = j.value("ancestor_id", 0u);
        generation_formed = j.value("gen_formed", 0u);
        member_count = j.value("members", 0u);
        if (j.contains("prototype")) {
            prototype_genome.from_json(j["prototype"]);
        }
    }
};

class SpeciationSystem {
public:
    struct SpeciationThresholds {
        double genetic_divergence_threshold{2.0};
        double phenotypic_divergence_threshold{1.5};
        double mating_compatibility_cutoff{0.25}; // Below this, reproduction is blocked
        double viability_scale{2.5};
    };

    explicit SpeciationSystem(const SpeciationThresholds& thresholds = SpeciationThresholds{})
        : m_thresholds(thresholds) {
        // Register species 0 (ancestral founding species)
        m_species[0] = {0, 0, 0, 0, Genome{}};
    }

    // 1. Genetic divergence
    [[nodiscard]] static double calculate_genetic_divergence(const Genome& a, const Genome& b) noexcept {
        return a.genetic_distance(b);
    }

    // 2. Phenotypic divergence (morphology and sensory)
    [[nodiscard]] static double calculate_phenotypic_divergence(const Genome& a, const Genome& b) noexcept {
        double d_mass = std::abs(a.body.mass - b.body.mass) / (0.5 * (a.body.mass + b.body.mass));
        double d_wing = std::abs(a.body.wing_span - b.body.wing_span) / (0.5 * (a.body.wing_span + b.body.wing_span));
        double d_dex = std::abs(a.body.manipulator_dexterity - b.body.manipulator_dexterity);
        return std::sqrt(d_mass * d_mass + d_wing * d_wing + d_dex * d_dex);
    }

    // 3. Mating compatibility: drops exponentially with genetic and phenotypic divergence
    [[nodiscard]] double calculate_mating_compatibility(const Genome& a, const Genome& b) const noexcept {
        double d_gen = calculate_genetic_divergence(a, b);
        double d_phen = calculate_phenotypic_divergence(a, b);

        double exp_arg = -((d_gen * d_gen) / (2.0 * m_thresholds.genetic_divergence_threshold * m_thresholds.genetic_divergence_threshold))
                         -((d_phen * d_phen) / (2.0 * m_thresholds.phenotypic_divergence_threshold * m_thresholds.phenotypic_divergence_threshold));
        return std::exp(exp_arg);
    }

    [[nodiscard]] bool can_interbreed(const Genome& a, const Genome& b) const noexcept {
        return calculate_mating_compatibility(a, b) >= m_thresholds.mating_compatibility_cutoff;
    }

    // 4. Offspring viability: viability probability for hybrid offspring
    [[nodiscard]] double calculate_offspring_viability(const Genome& a, const Genome& b) const noexcept {
        double d_gen = calculate_genetic_divergence(a, b);
        return std::exp(-(d_gen * d_gen) / (2.0 * m_thresholds.viability_scale * m_thresholds.viability_scale));
    }

    // 5. Detect species or classify individual into species cluster
    uint32_t classify_or_speciate(const Genome& g, uint32_t current_generation, uint32_t parent_species_id) {
        // Find best matching existing species
        double min_dist = 1e9;
        uint32_t best_species = parent_species_id;

        for (const auto& [sp_id, sp_info] : m_species) {
            double d = calculate_genetic_divergence(g, sp_info.prototype_genome);
            if (d < min_dist) {
                min_dist = d;
                best_species = sp_id;
            }
        }

        // If genetic distance to best matching species exceeds divergence threshold AND compatibility is broken:
        // Speciation event occurs!
        if (min_dist > m_thresholds.genetic_divergence_threshold &&
            !can_interbreed(g, m_species[best_species].prototype_genome)) {
            uint32_t new_id = m_next_species_id++;
            m_species[new_id] = {new_id, best_species, current_generation, 1, g};
            m_total_speciation_events++;
            return new_id;
        }

        m_species[best_species].member_count++;
        return best_species;
    }

    [[nodiscard]] size_t species_count() const noexcept { return m_species.size(); }
    [[nodiscard]] uint32_t total_speciation_events() const noexcept { return m_total_speciation_events; }
    [[nodiscard]] const std::unordered_map<uint32_t, SpeciesInfo>& all_species() const noexcept { return m_species; }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [k, v] : m_species) arr.push_back(v.to_json());
        return {
            {"next_species_id", m_next_species_id},
            {"total_speciations", m_total_speciation_events},
            {"species", arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_next_species_id = j.value("next_species_id", 1u);
        m_total_speciation_events = j.value("total_speciations", 0u);
        m_species.clear();
        if (j.contains("species")) {
            for (const auto& item : j["species"]) {
                SpeciesInfo info;
                info.from_json(item);
                m_species[info.species_id] = info;
            }
        }
    }

private:
    SpeciationThresholds m_thresholds;
    uint32_t m_next_species_id{1};
    uint32_t m_total_speciation_events{0};
    std::unordered_map<uint32_t, SpeciesInfo> m_species;
};

} // namespace flgod
