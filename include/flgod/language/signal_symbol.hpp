#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include "flgod/world/fields.hpp"
#include "flgod/core/entity_id.hpp"

namespace flgod::language {

enum class SignalChannel : uint8_t {
    Pheromone = 0,     // Chemical scent gradient
    AcousticWingBuzz,  // Frequency / buzz tempo modulation
    VisualDance,       // Waggle / figure-eight orientation and duration
    DirectTactile      // Antennae touch
};

inline const char* signal_channel_to_string(SignalChannel ch) {
    switch (ch) {
        case SignalChannel::Pheromone: return "Pheromone";
        case SignalChannel::AcousticWingBuzz: return "AcousticWingBuzz";
        case SignalChannel::VisualDance: return "VisualDance";
        case SignalChannel::DirectTactile: return "DirectTactile";
    }
    return "Unknown";
}

enum class SemanticReferent : uint8_t {
    Unknown = 0,
    FoodSource,         // High-sugar nectar / protein
    WaterSource,        // Hydration seep / dew drop
    DangerPredator,     // Threat alert / evasion warning
    NestColony,         // Home coordinates / hive entrance
    MatingInterest,     // Reproductive readiness
    FollowMe,           // Group movement / recruitment
    ResourceDepleted    // Warning that food patch is exhausted
};

inline const char* referent_to_string(SemanticReferent ref) {
    switch (ref) {
        case SemanticReferent::Unknown: return "Unknown";
        case SemanticReferent::FoodSource: return "FoodSource";
        case SemanticReferent::WaterSource: return "WaterSource";
        case SemanticReferent::DangerPredator: return "DangerPredator";
        case SemanticReferent::NestColony: return "NestColony";
        case SemanticReferent::MatingInterest: return "MatingInterest";
        case SemanticReferent::FollowMe: return "FollowMe";
        case SemanticReferent::ResourceDepleted: return "ResourceDepleted";
    }
    return "Unknown";
}

struct PhysicalSignal {
    SignalChannel channel{SignalChannel::AcousticWingBuzz};
    EntityID emitter_id{0};
    Vec3 emitter_position{0.0, 0.0, 0.0};
    double frequency_hz{250.0};       // Wingbeat frequency (~200-300 Hz for Drosophila)
    double intensity{1.0};            // Signal strength [0.0, 1.0]
    double duration_seconds{0.5};     // Temporal length
    double bearing_degrees{0.0};      // Heading / directional pointing
    uint32_t discrete_symbol_id{0};   // Associated symbol token (if grounded)
    double timestamp{0.0};
};

struct SymbolGrounding {
    uint32_t symbol_id{0};
    std::string token_str;
    SemanticReferent referent{SemanticReferent::Unknown};
    double confidence{0.5};          // Strength of association [0.0, 1.0]
    uint32_t usage_count{0};
    uint32_t success_count{0};

    [[nodiscard]] double success_rate() const noexcept {
        return (usage_count > 0) ? (static_cast<double>(success_count) / usage_count) : 0.0;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"symbol_id", symbol_id},
            {"token", token_str},
            {"referent", referent_to_string(referent)},
            {"confidence", confidence},
            {"usage_count", usage_count},
            {"success_count", success_count}
        };
    }
};

class SignalSymbolSystem {
private:
    std::unordered_map<uint32_t, SymbolGrounding> m_symbols;
    std::unordered_map<uint8_t, uint32_t> m_referent_to_symbol;

public:
    SignalSymbolSystem() {
        // Register core foundational symbols
        register_symbol(1, "SYM_FOOD_NECTAR", SemanticReferent::FoodSource, 0.9);
        register_symbol(2, "SYM_WATER_SEEP", SemanticReferent::WaterSource, 0.9);
        register_symbol(3, "SYM_ALERT_DANGER", SemanticReferent::DangerPredator, 0.95);
        register_symbol(4, "SYM_HOME_NEST", SemanticReferent::NestColony, 0.99);
        register_symbol(5, "SYM_RECRUIT_FOLLOW", SemanticReferent::FollowMe, 0.8);
        register_symbol(6, "SYM_DEPLETED", SemanticReferent::ResourceDepleted, 0.85);
    }

    void register_symbol(uint32_t id, const std::string& token, SemanticReferent ref, double initial_conf = 0.5) {
        m_symbols[id] = {id, token, ref, initial_conf, 0, 0};
        m_referent_to_symbol[static_cast<uint8_t>(ref)] = id;
    }

    [[nodiscard]] bool has_symbol(uint32_t id) const noexcept {
        return m_symbols.find(id) != m_symbols.end();
    }

    [[nodiscard]] const SymbolGrounding* get_symbol(uint32_t id) const noexcept {
        auto it = m_symbols.find(id);
        if (it != m_symbols.end()) return &it->second;
        return nullptr;
    }

    [[nodiscard]] uint32_t get_symbol_for_referent(SemanticReferent ref) const noexcept {
        auto it = m_referent_to_symbol.find(static_cast<uint8_t>(ref));
        if (it != m_referent_to_symbol.end()) return it->second;
        return 0;
    }

    // Reinforce or penalize association based on empirical interaction outcome
    void record_interaction_outcome(uint32_t symbol_id, bool outcome_successful, double learning_rate = 0.1) {
        auto it = m_symbols.find(symbol_id);
        if (it != m_symbols.end()) {
            it->second.usage_count++;
            if (outcome_successful) {
                it->second.success_count++;
                it->second.confidence = std::min(1.0, it->second.confidence + learning_rate * (1.0 - it->second.confidence));
            } else {
                it->second.confidence = std::max(0.05, it->second.confidence - learning_rate * it->second.confidence);
            }
        }
    }

    [[nodiscard]] size_t symbol_count() const noexcept { return m_symbols.size(); }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [_, s] : m_symbols) {
            arr.push_back(s.to_json());
        }
        return arr;
    }
};

} // namespace flgod::language
