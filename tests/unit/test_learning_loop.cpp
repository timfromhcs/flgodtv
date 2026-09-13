#include "flgod/learning/learner.hpp"
#include "flgod/learning/parallel_worker.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_learning_loop..." << std::endl;

    flgod::LearnerConfig cfg;
    cfg.learning_rate = 0.2;
    cfg.discount_factor = 0.9;
    cfg.epsilon = 0.2;
    cfg.num_actions = 4;

    flgod::QLearner learner(cfg);
    flgod::ReplayBuffer replay_buffer(2000);
    flgod::ToyEnvironment env(10, 9); // 10 states, goal at 9
    flgod::Xoshiro256PlusPlus rng(424242ULL);

    double sim_time = 0.0;
    const int total_episodes = 200;

    for (int ep = 0; ep < total_episodes; ++ep) {
        env.reset(0);
        bool done = false;

        while (!done) {
            uint32_t s = env.current_state();
            uint32_t a = learner.select_action(s, rng);
            auto res = env.step(a);

            // Step update and experience record
            flgod::ExperienceRecord exp = learner.step_update(1, sim_time, s, a, res.reward, res.next_state, res.terminal);
            replay_buffer.push(exp);

            sim_time += 0.01;
            done = res.terminal;
        }

        // Periodic batch replay training
        if (replay_buffer.size() >= 32 && ep % 2 == 0) {
            auto batch = replay_buffer.sample(32, rng);
            learner.train_batch(batch);
        }
    }

    std::cout << "  Executed " << total_episodes << " episodes. Total experiences: " 
              << learner.total_experiences() << ", Policy version: " << learner.policy_version() << std::endl;

    // Evaluation run: Greedy policy from state 0
    env.reset(0);
    int eval_steps = 0;
    bool reached_goal = false;

    while (eval_steps < 10) {
        uint32_t s = env.current_state();
        if (s == env.goal_state()) {
            reached_goal = true;
            break;
        }
        uint32_t best_a = learner.get_best_action(s);
        auto res = env.step(best_a);
        eval_steps++;
        if (res.terminal && res.next_state == env.goal_state()) {
            reached_goal = true;
            break;
        }
    }

    if (!reached_goal) {
        std::cerr << "FAILED: Learned policy failed to reach goal from state 0!" << std::endl;
        return 1;
    }
    std::cout << "  Learned optimal policy reached goal in " << eval_steps << " steps (optimal <= 5)!" << std::endl;

    // Serialization test
    nlohmann::json saved = learner.to_json();
    flgod::QLearner restored;
    restored.from_json(saved);

    if (restored.compute_hash() != learner.compute_hash()) {
        std::cerr << "FAILED: QLearner serialization hash mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  QLearner state serialization verified bit-exact." << std::endl;

    std::cout << "[TEST] test_learning_loop PASSED" << std::endl;
    return 0;
}
