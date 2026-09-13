#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

// 1. Body morphology traits
struct BodyGenes {
    double mass{1.0};                  // kg
    double wing_span{0.8};              // meters
    uint32_t leg_count{6};              // insects
    double manipulator_dexterity{0.5}; // [0.0, 1.0]
    double exoskeleton_thickness{0.02}; // meters
    double body_length{0.5};           // meters

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"mass", mass}, {"wing_span", wing_span}, {"legs", leg_count},
            {"dexterity", manipulator_dexterity}, {"exoskeleton", exoskeleton_thickness},
            {"length", body_length}
        };
    }
    void from_json(const nlohmann::json& j) {
        mass = j.value("mass", 1.0);
        wing_span = j.value("wing_span", 0.8);
        leg_count = j.value("legs", 6u);
        manipulator_dexterity = j.value("dexterity", 0.5);
        exoskeleton_thickness = j.value("exoskeleton", 0.02);
        body_length = j.value("length", 0.5);
    }
};

// 2. Metabolism traits
struct MetabolismGenes {
    double basal_metabolic_rate{1.0}; // Watts/energy units per tick
    double energy_efficiency{0.8};    // [0.0, 1.0]
    double starvation_tolerance{50.0}; // Max energy deficit before death
    double digestive_efficiency{0.75};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"bmr", basal_metabolic_rate}, {"efficiency", energy_efficiency},
            {"starvation_tol", starvation_tolerance}, {"digestive", digestive_efficiency}
        };
    }
    void from_json(const nlohmann::json& j) {
        basal_metabolic_rate = j.value("bmr", 1.0);
        energy_efficiency = j.value("efficiency", 0.8);
        starvation_tolerance = j.value("starvation_tol", 50.0);
        digestive_efficiency = j.value("digestive", 0.75);
    }
};

// 3. Sensory traits
struct SensoryGenes {
    double visual_acuity{1.0};        // Visual resolution factor
    double visual_fov_degrees{240.0};  // Compound eye FOV
    double olfactory_sensitivity{1.0}; // Scent perception radius
    double vibration_sensitivity{0.8};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"visual_acuity", visual_acuity}, {"fov", visual_fov_degrees},
            {"olfactory", olfactory_sensitivity}, {"vibration", vibration_sensitivity}
        };
    }
    void from_json(const nlohmann::json& j) {
        visual_acuity = j.value("visual_acuity", 1.0);
        visual_fov_degrees = j.value("fov", 240.0);
        olfactory_sensitivity = j.value("olfactory", 1.0);
        vibration_sensitivity = j.value("vibration", 0.8);
    }
};

// 4. Brain development traits (GEMINI.md Section 48)
struct BrainDevelopmentGenes {
    double brain_volume{1.0};                  // relative volume cm3
    uint64_t neuron_count{100000};             // Drosophila fly connectome scale (~140,000 neurons)
    double sensory_region_allocation{0.35};    // fraction of neurons in optic/antennal lobes
    double motor_region_allocation{0.25};      // central complex motor control
    double association_region_allocation{0.40}; // mushroom body learning/memory
    double connection_density{0.05};           // synaptic density
    double plasticity_rate{0.02};              // synaptic plasticity rate
    double metabolic_cost_per_neuron{1e-5};     // Energy cost per neuron per tick

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"volume", brain_volume}, {"neurons", neuron_count},
            {"sensory_alloc", sensory_region_allocation}, {"motor_alloc", motor_region_allocation},
            {"assoc_alloc", association_region_allocation}, {"conn_density", connection_density},
            {"plasticity", plasticity_rate}, {"cost_per_neuron", metabolic_cost_per_neuron}
        };
    }
    void from_json(const nlohmann::json& j) {
        brain_volume = j.value("volume", 1.0);
        neuron_count = j.value("neurons", 100000ULL);
        sensory_region_allocation = j.value("sensory_alloc", 0.35);
        motor_region_allocation = j.value("motor_alloc", 0.25);
        association_region_allocation = j.value("assoc_alloc", 0.40);
        connection_density = j.value("conn_density", 0.05);
        plasticity_rate = j.value("plasticity", 0.02);
        metabolic_cost_per_neuron = j.value("cost_per_neuron", 1e-5);
    }
};

// 5. Learning traits
struct LearningGenes {
    double base_learning_rate{0.1};
    double exploration_tendency{0.2};
    double discount_horizon{0.95};

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"lr", base_learning_rate}, {"explore", exploration_tendency}, {"discount", discount_horizon}};
    }
    void from_json(const nlohmann::json& j) {
        base_learning_rate = j.value("lr", 0.1);
        exploration_tendency = j.value("explore", 0.2);
        discount_horizon = j.value("discount", 0.95);
    }
};

// 6. Memory traits
struct MemoryGenes {
    uint32_t working_memory_capacity{7};
    uint32_t episodic_retention_ticks{5000};
    double consolidation_efficiency{0.8};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"wm_cap", working_memory_capacity},
            {"ep_ticks", episodic_retention_ticks},
            {"consolidation", consolidation_efficiency}
        };
    }
    void from_json(const nlohmann::json& j) {
        working_memory_capacity = j.value("wm_cap", 7u);
        episodic_retention_ticks = j.value("ep_ticks", 5000u);
        consolidation_efficiency = j.value("consolidation", 0.8);
    }
};

// 7. Social traits
struct SocialGenes {
    double social_curiosity{0.5};    // propensity to observe peers
    double imitation_tendency{0.6};   // propensity to imitate observed successful actions
    double aggression_threshold{0.4}; // defense vs tolerance

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"curiosity", social_curiosity}, {"imitation", imitation_tendency}, {"aggression", aggression_threshold}};
    }
    void from_json(const nlohmann::json& j) {
        social_curiosity = j.value("curiosity", 0.5);
        imitation_tendency = j.value("imitation", 0.6);
        aggression_threshold = j.value("aggression", 0.4);
    }
};

// 8. Reproductive traits
struct ReproductiveGenes {
    uint32_t maturation_age_ticks{300}; // age before sexually mature
    uint32_t max_offspring_count{4};
    double gestation_energy_cost{20.0};
    double parental_investment{0.3};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"maturation", maturation_age_ticks}, {"fecundity", max_offspring_count},
            {"gestation_cost", gestation_energy_cost}, {"investment", parental_investment}
        };
    }
    void from_json(const nlohmann::json& j) {
        maturation_age_ticks = j.value("maturation", 300u);
        max_offspring_count = j.value("fecundity", 4u);
        gestation_energy_cost = j.value("gestation_cost", 20.0);
        parental_investment = j.value("investment", 0.3);
    }
};

// Complete Genome
class Genome {
public:
    BodyGenes body{};
    MetabolismGenes metabolism{};
    SensoryGenes sensory{};
    BrainDevelopmentGenes brain{};
    LearningGenes learning{};
    MemoryGenes memory{};
    SocialGenes social{};
    ReproductiveGenes reproduction{};

    // Regulatory expression flags
    uint32_t regulatory_flags{0xFFFFFFFF};

    // Computes genetic distance between two genomes for speciation
    [[nodiscard]] double genetic_distance(const Genome& other) const noexcept {
        double dist = 0.0;
        auto d_sq = [](double a, double b) { double diff = a - b; return diff * diff; };

        dist += d_sq(body.mass, other.body.mass);
        dist += d_sq(body.wing_span, other.body.wing_span);
        dist += d_sq(body.manipulator_dexterity, other.body.manipulator_dexterity);
        dist += d_sq(metabolism.basal_metabolic_rate, other.metabolism.basal_metabolic_rate);
        dist += d_sq(sensory.visual_acuity, other.sensory.visual_acuity);
        dist += d_sq(sensory.olfactory_sensitivity, other.sensory.olfactory_sensitivity);
        dist += d_sq(brain.brain_volume, other.brain.brain_volume);
        dist += d_sq(brain.plasticity_rate, other.brain.plasticity_rate);
        dist += d_sq(learning.base_learning_rate, other.learning.base_learning_rate);
        dist += d_sq(social.social_curiosity, other.social.social_curiosity);
        dist += d_sq(social.aggression_threshold, other.social.aggression_threshold);

        // Regulatory bit difference penalty
        uint32_t bit_diff = regulatory_flags ^ other.regulatory_flags;
        int bits_differing = 0;
        while (bit_diff) {
            bits_differing += (bit_diff & 1);
            bit_diff >>= 1;
        }
        dist += bits_differing * 0.1;

        return std::sqrt(dist);
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_double = [&h](double v) {
            uint64_t b = 0;
            std::memcpy(&b, &v, sizeof(double));
            h ^= b;
            h *= 1099511628211ULL;
        };
        mix_double(body.mass);
        mix_double(body.wing_span);
        mix_double(body.manipulator_dexterity);
        mix_double(metabolism.basal_metabolic_rate);
        mix_double(brain.brain_volume);
        mix_double(brain.plasticity_rate);
        mix_double(learning.base_learning_rate);
        mix_double(social.social_curiosity);
        h ^= regulatory_flags;
        h *= 1099511628211ULL;
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"body", body.to_json()},
            {"metabolism", metabolism.to_json()},
            {"sensory", sensory.to_json()},
            {"brain", brain.to_json()},
            {"learning", learning.to_json()},
            {"memory", memory.to_json()},
            {"social", social.to_json()},
            {"reproduction", reproduction.to_json()},
            {"regulatory", regulatory_flags}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("body")) body.from_json(j["body"]);
        if (j.contains("metabolism")) metabolism.from_json(j["metabolism"]);
        if (j.contains("sensory")) sensory.from_json(j["sensory"]);
        if (j.contains("brain")) brain.from_json(j["brain"]);
        if (j.contains("learning")) learning.from_json(j["learning"]);
        if (j.contains("memory")) memory.from_json(j["memory"]);
        if (j.contains("social")) social.from_json(j["social"]);
        if (j.contains("reproduction")) reproduction.from_json(j["reproduction"]);
        regulatory_flags = j.value("regulatory", 0xFFFFFFFFu);
    }
};

} // namespace flgod
