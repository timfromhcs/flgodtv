#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <nlohmann/json.hpp>
#include "flgod/language/signal_symbol.hpp"

namespace flgod::language {

enum class SymbolCategory : uint8_t {
    Action = 0,     // E.g. Forage, Fly, Evade, Return
    Object = 1,     // E.g. Flower, Water, Predator, Nest
    Qualifier = 2,  // E.g. Abundant, Danger, Depleted
    Direction = 3   // E.g. North, South, Up, Down
};

struct VocabularyWord {
    uint32_t word_id{0};
    std::string token;
    SymbolCategory category{SymbolCategory::Object};
    uint32_t associated_symbol_id{0};
    uint64_t usage_frequency{0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"word_id", word_id},
            {"token", token},
            {"category", static_cast<int>(category)},
            {"symbol_id", associated_symbol_id},
            {"freq", usage_frequency}
        };
    }
};

struct SymbolicUtterance {
    std::vector<uint32_t> sequence; // Ordered list of word IDs
    EntityID speaker_id{0};
    double timestamp{0.0};

    [[nodiscard]] std::string to_string(const std::unordered_map<uint32_t, VocabularyWord>& vocab) const {
        std::ostringstream oss;
        for (size_t i = 0; i < sequence.size(); ++i) {
            auto it = vocab.find(sequence[i]);
            if (it != vocab.end()) {
                oss << it->second.token;
            } else {
                oss << "?" << sequence[i];
            }
            if (i + 1 < sequence.size()) oss << " ";
        }
        return oss.str();
    }
};

class VocabularySystem {
private:
    std::unordered_map<uint32_t, VocabularyWord> m_words;
    // Bigram transition counts: word_A -> (word_B -> count)
    std::unordered_map<uint64_t, uint32_t> m_bigram_transitions;
    uint64_t m_total_utterances{0};

public:
    VocabularySystem() {
        // Foundational grammar words
        register_word(101, "ACT_FORAGE", SymbolCategory::Action, 1);
        register_word(102, "ACT_EVADE", SymbolCategory::Action, 3);
        register_word(103, "ACT_RETURN", SymbolCategory::Action, 4);
        register_word(201, "OBJ_NECTAR", SymbolCategory::Object, 1);
        register_word(202, "OBJ_WATER", SymbolCategory::Object, 2);
        register_word(203, "OBJ_PREDATOR", SymbolCategory::Object, 3);
        register_word(204, "OBJ_NEST", SymbolCategory::Object, 4);
        register_word(301, "QUAL_ABUNDANT", SymbolCategory::Qualifier, 0);
        register_word(302, "QUAL_DEPLETED", SymbolCategory::Qualifier, 6);
        register_word(401, "DIR_NORTH", SymbolCategory::Direction, 0);
        register_word(402, "DIR_SOUTH", SymbolCategory::Direction, 0);
    }

    void register_word(uint32_t id, const std::string& token, SymbolCategory cat, uint32_t sym_id = 0) {
        m_words[id] = {id, token, cat, sym_id, 0};
    }

    [[nodiscard]] size_t vocabulary_size() const noexcept { return word_count(); }

    [[nodiscard]] bool has_word(uint32_t id) const noexcept {
        return m_words.find(id) != m_words.end();
    }

    [[nodiscard]] const VocabularyWord* get_word(uint32_t id) const noexcept {
        auto it = m_words.find(id);
        if (it != m_words.end()) return &it->second;
        return nullptr;
    }

    // Records a multi-token utterance and updates grammatical sequence patterns
    void record_utterance(const SymbolicUtterance& utterance) {
        if (utterance.sequence.empty()) return;

        m_total_utterances++;
        for (size_t i = 0; i < utterance.sequence.size(); ++i) {
            uint32_t w_id = utterance.sequence[i];
            auto it = m_words.find(w_id);
            if (it != m_words.end()) {
                it->second.usage_frequency++;
            }

            // Bigram sequence pattern update
            if (i + 1 < utterance.sequence.size()) {
                uint32_t next_id = utterance.sequence[i + 1];
                uint64_t bigram_key = (static_cast<uint64_t>(w_id) << 32) | next_id;
                m_bigram_transitions[bigram_key]++;
            }
        }
    }

    // Computes bigram transition probability P(W_B | W_A)
    [[nodiscard]] double get_transition_probability(uint32_t word_a, uint32_t word_b) const noexcept {
        uint64_t key = (static_cast<uint64_t>(word_a) << 32) | word_b;
        auto it = m_bigram_transitions.find(key);
        if (it == m_bigram_transitions.end()) return 0.0;

        auto w_it = m_words.find(word_a);
        if (w_it == m_words.end() || w_it->second.usage_frequency == 0) return 0.0;

        return static_cast<double>(it->second) / w_it->second.usage_frequency;
    }

    // Validates grammar against canonical structure: [Action] [Object] [Qualifier/Direction optional]
    [[nodiscard]] bool validate_syntax(const std::vector<uint32_t>& sequence) const {
        if (sequence.size() < 2 || sequence.size() > 4) return false;

        auto w0 = get_word(sequence[0]);
        auto w1 = get_word(sequence[1]);
        if (!w0 || !w1) return false;

        // Pattern: Action followed by Object
        if (w0->category != SymbolCategory::Action || w1->category != SymbolCategory::Object) {
            return false;
        }

        // Optional 3rd word: Qualifier or Direction
        if (sequence.size() >= 3) {
            auto w2 = get_word(sequence[2]);
            if (!w2 || (w2->category != SymbolCategory::Qualifier && w2->category != SymbolCategory::Direction)) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] size_t word_count() const noexcept { return m_words.size(); }
    [[nodiscard]] uint64_t total_utterances() const noexcept { return m_total_utterances; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        for (const auto& [id, w] : m_words) {
            h ^= id;
            h *= 1099511628211ULL;
            h ^= w.usage_frequency;
            h *= 1099511628211ULL;
        }
        for (const auto& [bg, cnt] : m_bigram_transitions) {
            h ^= bg;
            h *= 1099511628211ULL;
            h ^= cnt;
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json words_arr = nlohmann::json::array();
        for (const auto& [_, w] : m_words) words_arr.push_back(w.to_json());

        nlohmann::json bigrams = nlohmann::json::array();
        for (const auto& [k, c] : m_bigram_transitions) {
            bigrams.push_back({{"key", k}, {"count", c}});
        }

        return {
            {"words", words_arr},
            {"bigrams", bigrams},
            {"total_utterances", m_total_utterances}
        };
    }
};

} // namespace flgod::language
