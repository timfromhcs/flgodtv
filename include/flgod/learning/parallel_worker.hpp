#pragma once

#include "flgod/learning/experience.hpp"
#include "flgod/learning/learner.hpp"
#include "flgod/learning/memory.hpp"
#include "flgod/core/simulation.hpp"
#include <vector>
#include <memory>
#include <mutex>

namespace flgod {

// Toy Environment for verified sample efficiency benchmarks (GEMINI.md Section 32 & 33)
// 1D/2D grid discrete navigation: Start at state 0, goal at state N - 1.
// Actions: 0: Left, 1: Right, 2: Stay, 3: Sprint Right
class ToyEnvironment {
public:
    explicit ToyEnvironment(uint32_t num_states = 10, uint32_t goal_state = 9)
        : m_num_states(num_states), m_goal_state(goal_state) {}

    void reset(uint32_t start_state = 0) {
        m_current_state = start_state;
        m_step_count = 0;
    }

    [[nodiscard]] uint32_t current_state() const noexcept { return m_current_state; }
    [[nodiscard]] uint32_t goal_state() const noexcept { return m_goal_state; }
    void set_goal_state(uint32_t g) noexcept { m_goal_state = g; }

    struct StepResult {
        uint32_t next_state;
        double reward;
        bool terminal;
    };

    StepResult step(uint32_t action) {
        m_step_count++;
        int next = static_cast<int>(m_current_state);

        switch (action) {
            case 0: next = std::max(0, next - 1); break; // Left
            case 1: next = std::min(static_cast<int>(m_num_states - 1), next + 1); break; // Right
            case 2: break; // Stay
            case 3: next = std::min(static_cast<int>(m_num_states - 1), next + 2); break; // Sprint
            default: break;
        }

        m_current_state = static_cast<uint32_t>(next);
        bool terminal = (m_current_state == m_goal_state);
        double reward = terminal ? 10.0 : -0.1; // goal reward vs small time penalty

        if (m_step_count >= 50) {
            terminal = true; // truncation limit
        }

        return {m_current_state, reward, terminal};
    }

private:
    uint32_t m_num_states{10};
    uint32_t m_goal_state{9};
    uint32_t m_current_state{0};
    uint32_t m_step_count{0};
};

class ExperienceCollector {
public:
    void push(const ExperienceRecord& exp) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(exp);
    }

    std::vector<ExperienceRecord> drain() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ExperienceRecord> out = std::move(m_queue);
        m_queue.clear();
        return out;
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    mutable std::mutex m_mutex;
    std::vector<ExperienceRecord> m_queue;
};

class AgentWorker {
public:
    explicit AgentWorker(uint64_t agent_id, QLearner& learner)
        : m_agent_id(agent_id), m_learner(learner) {}

    uint32_t act(uint32_t state, Xoshiro256PlusPlus& rng) {
        return m_learner.select_action(state, rng);
    }

    void remember(const ExperienceRecord& exp, ExperienceCollector& collector) {
        m_memory.episodic().record_event(exp.agent_id, exp.timestamp, exp.observation.state_id, exp.action.discrete_action, exp.reward);
        collector.push(exp);
    }

    [[nodiscard]] MemorySystem& memory() noexcept { return m_memory; }
    [[nodiscard]] uint64_t id() const noexcept { return m_agent_id; }

private:
    uint64_t m_agent_id{0};
    QLearner& m_learner;
    MemorySystem m_memory;
};

class EnvironmentWorker {
public:
    explicit EnvironmentWorker(uint32_t worker_id, uint32_t num_states = 10)
        : m_worker_id(worker_id), m_env(num_states) {}

    ToyEnvironment& env() noexcept { return m_env; }

    void run_episode(AgentWorker& agent, ExperienceCollector& collector, Xoshiro256PlusPlus& rng, double start_time) {
        m_env.reset();
        bool done = false;
        double current_time = start_time;

        while (!done) {
            uint32_t s = m_env.current_state();
            uint32_t a = agent.act(s, rng);
            auto res = m_env.step(a);

            ExperienceRecord exp;
            exp.agent_id = agent.id();
            exp.timestamp = current_time;
            exp.observation.state_id = s;
            exp.action.discrete_action = a;
            exp.reward = res.reward;
            exp.outcome.next_state_id = res.next_state;
            exp.outcome.is_terminal = res.terminal;

            agent.remember(exp, collector);

            current_time += 0.05;
            done = res.terminal;
        }
    }

private:
    uint32_t m_worker_id{0};
    ToyEnvironment m_env;
};

} // namespace flgod
