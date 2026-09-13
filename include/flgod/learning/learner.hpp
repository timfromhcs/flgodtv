#pragma once

#include "flgod/learning/experience.hpp"
#include "flgod/learning/memory.hpp"
#include "flgod/core/rng.hpp"
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

struct LearnerConfig {
    double learning_rate{0.1};
    double discount_factor{0.95};
    double epsilon{0.1}; // exploration rate
    uint32_t num_actions{4};
    uint32_t batch_size{32};
};

class QLearner {
public:
    explicit QLearner(const LearnerConfig& config = LearnerConfig{})
        : m_config(config) {}

    [[nodiscard]] double get_q(uint32_t state, uint32_t action) const noexcept {
        uint64_t key = (static_cast<uint64_t>(state) << 32) | action;
        auto it = m_q_table.find(key);
        return (it != m_q_table.end()) ? it->second : 0.0;
    }

    void set_q(uint32_t state, uint32_t action, double val) noexcept {
        uint64_t key = (static_cast<uint64_t>(state) << 32) | action;
        m_q_table[key] = val;
    }

    [[nodiscard]] double get_max_q(uint32_t state) const noexcept {
        double max_val = -1e9;
        for (uint32_t a = 0; a < m_config.num_actions; ++a) {
            double q = get_q(state, a);
            if (q > max_val) max_val = q;
        }
        return (max_val > -1e8) ? max_val : 0.0;
    }

    [[nodiscard]] uint32_t select_action(uint32_t state, Xoshiro256PlusPlus& rng) const {
        if (rng.next_double() < m_config.epsilon) {
            return static_cast<uint32_t>(rng.next_u64() % m_config.num_actions);
        }
        return get_best_action(state);
    }

    [[nodiscard]] uint32_t get_best_action(uint32_t state) const noexcept {
        double max_val = -1e9;
        uint32_t best_act = 0;
        for (uint32_t a = 0; a < m_config.num_actions; ++a) {
            double q = get_q(state, a);
            if (q > max_val) {
                max_val = q;
                best_act = a;
            }
        }
        return best_act;
    }

    // Step update according to GEMINI.md Section 32:
    // observe -> predict -> act -> observe outcome -> calculate error -> store -> update
    ExperienceRecord step_update(uint64_t agent_id, double timestamp,
                                 uint32_t state, uint32_t action,
                                 double reward, uint32_t next_state, bool terminal) {
        double prediction = get_q(state, action);
        double target = reward;
        if (!terminal) {
            target += m_config.discount_factor * get_max_q(next_state);
        }
        double td_error = target - prediction;

        // Q-table update
        double new_q = prediction + m_config.learning_rate * td_error;
        set_q(state, action, new_q);

        m_total_experiences++;
        m_cumulative_td_error += std::abs(td_error);

        ExperienceRecord rec;
        rec.agent_id = agent_id;
        rec.policy_version = m_policy_version;
        rec.timestamp = timestamp;
        rec.observation.state_id = state;
        rec.action.discrete_action = action;
        rec.reward = reward;
        rec.outcome.next_state_id = next_state;
        rec.outcome.is_terminal = terminal;
        rec.prediction = prediction;
        rec.prediction_error = td_error;

        return rec;
    }

    // Train on batch from replay buffer
    double train_batch(const std::vector<ExperienceRecord>& batch) {
        if (batch.empty()) return 0.0;
        double batch_err = 0.0;

        for (const auto& rec : batch) {
            uint32_t s = rec.observation.state_id;
            uint32_t a = rec.action.discrete_action;
            double r = rec.reward;
            uint32_t s_next = rec.outcome.next_state_id;
            bool term = rec.outcome.is_terminal;

            double cur_q = get_q(s, a);
            double target = r;
            if (!term) {
                target += m_config.discount_factor * get_max_q(s_next);
            }
            double err = target - cur_q;
            set_q(s, a, cur_q + m_config.learning_rate * err);
            batch_err += std::abs(err);
        }

        m_policy_version++;
        return batch_err / batch.size();
    }

    [[nodiscard]] uint32_t policy_version() const noexcept { return m_policy_version; }
    [[nodiscard]] uint64_t total_experiences() const noexcept { return m_total_experiences; }
    [[nodiscard]] double average_td_error() const noexcept {
        return (m_total_experiences > 0) ? (m_cumulative_td_error / m_total_experiences) : 0.0;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        // Deterministic hash over sorted keys
        std::vector<uint64_t> keys;
        keys.reserve(m_q_table.size());
        for (const auto& [k, _] : m_q_table) keys.push_back(k);
        std::sort(keys.begin(), keys.end());

        for (uint64_t k : keys) {
            h ^= k;
            h *= 1099511628211ULL;
            double v = m_q_table.at(k);
            uint64_t b = 0;
            std::memcpy(&b, &v, sizeof(double));
            h ^= b;
            h *= 1099511628211ULL;
        }
        h ^= m_policy_version;
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json table_arr = nlohmann::json::array();
        for (const auto& [k, v] : m_q_table) {
            table_arr.push_back({{"key", k}, {"val", v}});
        }
        return {
            {"policy_ver", m_policy_version},
            {"experiences", m_total_experiences},
            {"cum_error", m_cumulative_td_error},
            {"q_table", table_arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_policy_version = j.value("policy_ver", 1u);
        m_total_experiences = j.value("experiences", 0ULL);
        m_cumulative_td_error = j.value("cum_error", 0.0);
        m_q_table.clear();
        if (j.contains("q_table")) {
            for (const auto& item : j["q_table"]) {
                uint64_t k = item.value("key", 0ULL);
                double v = item.value("val", 0.0);
                m_q_table[k] = v;
            }
        }
    }

private:
    LearnerConfig m_config{};
    std::unordered_map<uint64_t, double> m_q_table;
    uint32_t m_policy_version{1};
    uint64_t m_total_experiences{0};
    double m_cumulative_td_error{0.0};
};

} // namespace flgod
