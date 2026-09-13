#include "flgod/learning/parallel_worker.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_learning_parallel..." << std::endl;

    flgod::LearnerConfig cfg;
    cfg.learning_rate = 0.2;
    cfg.discount_factor = 0.9;
    cfg.epsilon = 0.2;
    cfg.num_actions = 4;

    flgod::QLearner central_learner(cfg);
    flgod::ExperienceCollector collector;

    const size_t num_workers = 4;
    std::vector<flgod::EnvironmentWorker> env_workers;
    std::vector<flgod::AgentWorker> agent_workers;
    std::vector<flgod::Xoshiro256PlusPlus> worker_rngs;

    env_workers.reserve(num_workers);
    agent_workers.reserve(num_workers);
    worker_rngs.reserve(num_workers);

    for (size_t i = 0; i < num_workers; ++i) {
        env_workers.emplace_back(static_cast<uint32_t>(i), 10);
        agent_workers.emplace_back(static_cast<uint64_t>(100 + i), central_learner);
        worker_rngs.emplace_back(777000ULL + i * 111ULL);
    }

    // Run parallel rounds of episodes
    double global_time = 0.0;
    const int rounds = 25;

    for (int r = 0; r < rounds; ++r) {
        for (size_t i = 0; i < num_workers; ++i) {
            env_workers[i].run_episode(agent_workers[i], collector, worker_rngs[i], global_time);
            global_time += 1.0;
        }

        // Central learner drains experiences and updates policy
        auto exps = collector.drain();
        central_learner.train_batch(exps);
    }

    std::cout << "  Parallel training finished: " << central_learner.policy_version()
              << " policy updates across " << num_workers << " workers." << std::endl;

    // Test memory consolidation on agent workers
    for (size_t i = 0; i < num_workers; ++i) {
        agent_workers[i].memory().consolidate();
        if (agent_workers[i].memory().episodic().size() == 0) {
            std::cerr << "FAILED: Agent worker " << i << " did not record episodic experiences!" << std::endl;
            return 1;
        }
    }
    std::cout << "  Agent workers memory consolidated successfully." << std::endl;

    // Verification evaluation
    flgod::ToyEnvironment eval_env(10, 9);
    eval_env.reset(0);
    int steps = 0;
    bool reached = false;

    while (steps < 10) {
        uint32_t s = eval_env.current_state();
        if (s == eval_env.goal_state()) {
            reached = true;
            break;
        }
        uint32_t a = central_learner.get_best_action(s);
        auto res = eval_env.step(a);
        steps++;
        if (res.terminal && res.next_state == eval_env.goal_state()) {
            reached = true;
            break;
        }
    }

    if (!reached) {
        std::cerr << "FAILED: Policy trained via parallel workers failed evaluation!" << std::endl;
        return 1;
    }
    std::cout << "  Parallel trained policy reached goal in " << steps << " steps (success)!" << std::endl;

    std::cout << "[TEST] test_learning_parallel PASSED" << std::endl;
    return 0;
}
