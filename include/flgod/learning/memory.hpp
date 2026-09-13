#pragma once

#include "flgod/learning/experience.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <deque>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

// 1. Working Memory: immediate active capacity
struct WorkingMemoryItem {
    std::string tag;
    std::vector<double> vector_data;
    double activation{1.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"tag", tag}, {"data", vector_data}, {"act", activation}};
    }
    void from_json(const nlohmann::json& j) {
        tag = j.value("tag", "");
        if (j.contains("data")) vector_data = j["data"].get<std::vector<double>>();
        activation = j.value("act", 1.0);
    }
};

class WorkingMemory {
public:
    explicit WorkingMemory(size_t max_items = 7) : m_max_items(max_items) {}

    void add_item(const std::string& tag, const std::vector<double>& data) {
        if (m_items.size() >= m_max_items) {
            m_items.pop_front();
        }
        m_items.push_back({tag, data, 1.0});
    }

    [[nodiscard]] const std::deque<WorkingMemoryItem>& items() const noexcept { return m_items; }
    [[nodiscard]] size_t size() const noexcept { return m_items.size(); }
    void clear() noexcept { m_items.clear(); }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& it : m_items) arr.push_back(it.to_json());
        return {{"max_items", m_max_items}, {"items", arr}};
    }
    void from_json(const nlohmann::json& j) {
        m_max_items = j.value("max_items", 7ULL);
        m_items.clear();
        if (j.contains("items")) {
            for (const auto& it : j["items"]) {
                WorkingMemoryItem item;
                item.from_json(it);
                m_items.push_back(item);
            }
        }
    }

private:
    size_t m_max_items{7};
    std::deque<WorkingMemoryItem> m_items;
};

// 2. Episodic Memory: autobiographical experience episodes
struct EpisodeRecord {
    uint64_t episode_id{0};
    double timestamp{0.0};
    uint32_t state_id{0};
    uint32_t action_id{0};
    double reward{0.0};
    double emotional_valence{0.0}; // -1.0 to 1.0

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"ep_id", episode_id},
            {"time", timestamp},
            {"state", state_id},
            {"act", action_id},
            {"rew", reward},
            {"valence", emotional_valence}
        };
    }
    void from_json(const nlohmann::json& j) {
        episode_id = j.value("ep_id", 0ULL);
        timestamp = j.value("time", 0.0);
        state_id = j.value("state", 0u);
        action_id = j.value("act", 0u);
        reward = j.value("rew", 0.0);
        emotional_valence = j.value("valence", 0.0);
    }
};

class EpisodicMemory {
public:
    explicit EpisodicMemory(size_t max_episodes = 5000) : m_max_episodes(max_episodes) {}

    void record_event(uint64_t ep_id, double time, uint32_t state, uint32_t action, double reward) {
        if (m_episodes.size() >= m_max_episodes) {
            m_episodes.pop_front();
        }
        double valence = std::tanh(reward);
        m_episodes.push_back({ep_id, time, state, action, reward, valence});
    }

    [[nodiscard]] const std::deque<EpisodeRecord>& episodes() const noexcept { return m_episodes; }
    [[nodiscard]] size_t size() const noexcept { return m_episodes.size(); }

    [[nodiscard]] std::vector<EpisodeRecord> query_by_state(uint32_t state_id) const {
        std::vector<EpisodeRecord> matches;
        for (const auto& ep : m_episodes) {
            if (ep.state_id == state_id) matches.push_back(ep);
        }
        return matches;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& ep : m_episodes) arr.push_back(ep.to_json());
        return {{"max_episodes", m_max_episodes}, {"episodes", arr}};
    }
    void from_json(const nlohmann::json& j) {
        m_max_episodes = j.value("max_episodes", 5000ULL);
        m_episodes.clear();
        if (j.contains("episodes")) {
            for (const auto& it : j["episodes"]) {
                EpisodeRecord ep;
                ep.from_json(it);
                m_episodes.push_back(ep);
            }
        }
    }

private:
    size_t m_max_episodes{5000};
    std::deque<EpisodeRecord> m_episodes;
};

// 3. Semantic Memory: generalized knowledge & concept association
struct SemanticFact {
    std::string concept_key;
    double confidence{1.0};
    std::vector<double> attributes{};

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"key", concept_key}, {"conf", confidence}, {"attrs", attributes}};
    }
    void from_json(const nlohmann::json& j) {
        concept_key = j.value("key", "");
        confidence = j.value("conf", 1.0);
        if (j.contains("attrs")) attributes = j["attrs"].get<std::vector<double>>();
    }
};

class SemanticMemory {
public:
    void store_fact(const std::string& key, const std::vector<double>& attributes, double confidence = 1.0) {
        m_facts[key] = {key, confidence, attributes};
    }

    [[nodiscard]] bool has_fact(const std::string& key) const noexcept {
        return m_facts.find(key) != m_facts.end();
    }

    [[nodiscard]] const SemanticFact& get_fact(const std::string& key) const {
        return m_facts.at(key);
    }

    [[nodiscard]] size_t fact_count() const noexcept { return m_facts.size(); }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [k, fact] : m_facts) {
            j[k] = fact.to_json();
        }
        return j;
    }
    void from_json(const nlohmann::json& j) {
        m_facts.clear();
        for (auto it = j.begin(); it != j.end(); ++it) {
            SemanticFact fact;
            fact.from_json(it.value());
            m_facts[it.key()] = fact;
        }
    }

private:
    std::unordered_map<std::string, SemanticFact> m_facts;
};

// 4. Procedural Memory: learned skills & motor habits
class ProceduralMemory {
public:
    void record_habit(uint32_t state, uint32_t action, double weight_delta) {
        uint64_t key = (static_cast<uint64_t>(state) << 32) | action;
        m_habits[key] += weight_delta;
    }

    [[nodiscard]] double get_habit_strength(uint32_t state, uint32_t action) const noexcept {
        uint64_t key = (static_cast<uint64_t>(state) << 32) | action;
        auto it = m_habits.find(key);
        return (it != m_habits.end()) ? it->second : 0.0;
    }

    [[nodiscard]] uint32_t get_best_action(uint32_t state, uint32_t num_actions, uint32_t default_action = 0) const noexcept {
        double max_val = -1e9;
        uint32_t best_act = default_action;
        for (uint32_t a = 0; a < num_actions; ++a) {
            double strength = get_habit_strength(state, a);
            if (strength > max_val) {
                max_val = strength;
                best_act = a;
            }
        }
        return best_act;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [k, v] : m_habits) {
            arr.push_back({{"key", k}, {"weight", v}});
        }
        return arr;
    }
    void from_json(const nlohmann::json& j) {
        m_habits.clear();
        for (const auto& item : j) {
            uint64_t k = item.value("key", 0ULL);
            double w = item.value("weight", 0.0);
            m_habits[k] = w;
        }
    }

private:
    std::unordered_map<uint64_t, double> m_habits;
};

// 5. Social Memory: interactions and reputation of other entities
struct SocialRecord {
    uint64_t peer_id{0};
    double trust_score{0.0}; // -1.0 to 1.0
    uint32_t interaction_count{0};
    double last_interaction_time{0.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"peer_id", peer_id},
            {"trust", trust_score},
            {"count", interaction_count},
            {"last_time", last_interaction_time}
        };
    }
    void from_json(const nlohmann::json& j) {
        peer_id = j.value("peer_id", 0ULL);
        trust_score = j.value("trust", 0.0);
        interaction_count = j.value("count", 0u);
        last_interaction_time = j.value("last_time", 0.0);
    }
};

class SocialMemory {
public:
    void update_interaction(uint64_t peer_id, double valence, double time) {
        auto& rec = m_records[peer_id];
        rec.peer_id = peer_id;
        rec.interaction_count++;
        rec.last_interaction_time = time;
        // Exponential moving average update of trust score
        rec.trust_score = rec.trust_score * 0.8 + valence * 0.2;
        rec.trust_score = std::clamp(rec.trust_score, -1.0, 1.0);
    }

    [[nodiscard]] double get_trust(uint64_t peer_id) const noexcept {
        auto it = m_records.find(peer_id);
        return (it != m_records.end()) ? it->second.trust_score : 0.0;
    }

    [[nodiscard]] size_t peer_count() const noexcept { return m_records.size(); }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [k, v] : m_records) arr.push_back(v.to_json());
        return arr;
    }
    void from_json(const nlohmann::json& j) {
        m_records.clear();
        for (const auto& item : j) {
            SocialRecord rec;
            rec.from_json(item);
            m_records[rec.peer_id] = rec;
        }
    }

private:
    std::unordered_map<uint64_t, SocialRecord> m_records;
};

// Unified Multi-Tier Memory System
class MemorySystem {
public:
    MemorySystem() = default;

    [[nodiscard]] WorkingMemory& working() noexcept { return m_working; }
    [[nodiscard]] const WorkingMemory& working() const noexcept { return m_working; }

    [[nodiscard]] EpisodicMemory& episodic() noexcept { return m_episodic; }
    [[nodiscard]] const EpisodicMemory& episodic() const noexcept { return m_episodic; }

    [[nodiscard]] SemanticMemory& semantic() noexcept { return m_semantic; }
    [[nodiscard]] const SemanticMemory& semantic() const noexcept { return m_semantic; }

    [[nodiscard]] ProceduralMemory& procedural() noexcept { return m_procedural; }
    [[nodiscard]] const ProceduralMemory& procedural() const noexcept { return m_procedural; }

    [[nodiscard]] SocialMemory& social() noexcept { return m_social; }
    [[nodiscard]] const SocialMemory& social() const noexcept { return m_social; }

    // Memory Consolidation: Transfers high-salience episodes into semantic facts & procedural habits
    void consolidate() {
        for (const auto& ep : m_episodic.episodes()) {
            if (std::abs(ep.reward) > 0.5) {
                // Reinforce procedural habit
                m_procedural.record_habit(ep.state_id, ep.action_id, ep.reward * 0.1);

                // Store/update semantic fact for significant outcomes
                std::string concept_key = "state_" + std::to_string(ep.state_id) + "_act_" + std::to_string(ep.action_id);
                m_semantic.store_fact(concept_key, {ep.reward, ep.emotional_valence}, 0.95);
            }
        }
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix(m_working.size());
        mix(m_episodic.size());
        mix(m_semantic.fact_count());
        mix(m_social.peer_count());
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"working", m_working.to_json()},
            {"episodic", m_episodic.to_json()},
            {"semantic", m_semantic.to_json()},
            {"procedural", m_procedural.to_json()},
            {"social", m_social.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("working")) m_working.from_json(j["working"]);
        if (j.contains("episodic")) m_episodic.from_json(j["episodic"]);
        if (j.contains("semantic")) m_semantic.from_json(j["semantic"]);
        if (j.contains("procedural")) m_procedural.from_json(j["procedural"]);
        if (j.contains("social")) m_social.from_json(j["social"]);
    }

private:
    WorkingMemory m_working;
    EpisodicMemory m_episodic;
    SemanticMemory m_semantic;
    ProceduralMemory m_procedural;
    SocialMemory m_social;
};

} // namespace flgod
