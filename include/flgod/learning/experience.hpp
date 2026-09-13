#pragma once

#include "flgod/core/rng.hpp"
#include "flgod/core/entity_id.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <deque>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

struct Observation {
    uint32_t state_id{0};
    std::vector<double> features{};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"state_id", state_id},
            {"features", features}
        };
    }

    void from_json(const nlohmann::json& j) {
        state_id = j.value("state_id", 0u);
        if (j.contains("features")) {
            features = j["features"].get<std::vector<double>>();
        }
    }
};

struct Action {
    uint32_t discrete_action{0};
    std::vector<double> continuous_actions{};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"discrete", discrete_action},
            {"continuous", continuous_actions}
        };
    }

    void from_json(const nlohmann::json& j) {
        discrete_action = j.value("discrete", 0u);
        if (j.contains("continuous")) {
            continuous_actions = j["continuous"].get<std::vector<double>>();
        }
    }
};

struct Outcome {
    uint32_t next_state_id{0};
    std::vector<double> next_features{};
    bool is_terminal{false};
    bool is_truncated{false};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"next_state", next_state_id},
            {"next_features", next_features},
            {"terminal", is_terminal},
            {"truncated", is_truncated}
        };
    }

    void from_json(const nlohmann::json& j) {
        next_state_id = j.value("next_state", 0u);
        if (j.contains("next_features")) {
            next_features = j["next_features"].get<std::vector<double>>();
        }
        is_terminal = j.value("terminal", false);
        is_truncated = j.value("truncated", false);
    }
};

struct ExperienceRecord {
    uint64_t agent_id{0};
    uint32_t policy_version{1};
    double timestamp{0.0};
    Observation observation{};
    Action action{};
    double reward{0.0};
    Outcome outcome{};
    double prediction{0.0};
    double prediction_error{0.0};
    std::string world_context{"default"};

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_u64 = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        auto mix_double = [&h, &mix_u64](double v) {
            uint64_t b = 0;
            std::memcpy(&b, &v, sizeof(double));
            mix_u64(b);
        };
        mix_u64(agent_id);
        mix_u64(policy_version);
        mix_double(timestamp);
        mix_u64(observation.state_id);
        mix_u64(action.discrete_action);
        mix_double(reward);
        mix_u64(outcome.next_state_id);
        mix_double(prediction);
        mix_double(prediction_error);
        for (char c : world_context) {
            mix_u64(static_cast<uint8_t>(c));
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"agent_id", agent_id},
            {"policy_ver", policy_version},
            {"time", timestamp},
            {"obs", observation.to_json()},
            {"act", action.to_json()},
            {"reward", reward},
            {"outcome", outcome.to_json()},
            {"prediction", prediction},
            {"pred_err", prediction_error},
            {"context", world_context}
        };
    }

    void from_json(const nlohmann::json& j) {
        agent_id = j.value("agent_id", 0ULL);
        policy_version = j.value("policy_ver", 1u);
        timestamp = j.value("time", 0.0);
        if (j.contains("obs")) observation.from_json(j["obs"]);
        if (j.contains("act")) action.from_json(j["act"]);
        reward = j.value("reward", 0.0);
        if (j.contains("outcome")) outcome.from_json(j["outcome"]);
        prediction = j.value("prediction", 0.0);
        prediction_error = j.value("pred_err", 0.0);
        world_context = j.value("context", "default");
    }
};

class ReplayBuffer {
public:
    explicit ReplayBuffer(size_t capacity = 10000)
        : m_capacity(capacity) {}

    void push(const ExperienceRecord& record) {
        if (m_buffer.size() >= m_capacity) {
            m_buffer.pop_front();
        }
        m_buffer.push_back(record);
    }

    [[nodiscard]] size_t size() const noexcept { return m_buffer.size(); }
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
    [[nodiscard]] bool empty() const noexcept { return m_buffer.empty(); }

    void clear() noexcept { m_buffer.clear(); }

    [[nodiscard]] const ExperienceRecord& operator[](size_t index) const {
        return m_buffer[index];
    }

    std::vector<ExperienceRecord> sample(size_t batch_size, Xoshiro256PlusPlus& rng) const {
        std::vector<ExperienceRecord> batch;
        if (m_buffer.empty()) return batch;

        batch.reserve(batch_size);
        for (size_t i = 0; i < batch_size; ++i) {
            size_t idx = static_cast<size_t>(rng.next_u64() % m_buffer.size());
            batch.push_back(m_buffer[idx]);
        }
        return batch;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        for (const auto& rec : m_buffer) {
            h ^= rec.compute_hash();
            h *= 1099511628211ULL;
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& rec : m_buffer) {
            arr.push_back(rec.to_json());
        }
        return {
            {"capacity", m_capacity},
            {"buffer", arr}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_capacity = j.value("capacity", 10000ULL);
        m_buffer.clear();
        if (j.contains("buffer")) {
            for (const auto& item : j["buffer"]) {
                ExperienceRecord rec;
                rec.from_json(item);
                m_buffer.push_back(rec);
            }
        }
    }

private:
    size_t m_capacity{10000};
    std::deque<ExperienceRecord> m_buffer;
};

} // namespace flgod
