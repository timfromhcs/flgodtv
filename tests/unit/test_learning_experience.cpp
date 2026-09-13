#include "flgod/learning/experience.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "[TEST] Running test_learning_experience..." << std::endl;

    flgod::ExperienceRecord rec;
    rec.agent_id = 42;
    rec.policy_version = 2;
    rec.timestamp = 1.25;
    rec.observation.state_id = 7;
    rec.observation.features = {0.5, -0.2, 1.0};
    rec.action.discrete_action = 3;
    rec.reward = 5.0;
    rec.outcome.next_state_id = 9;
    rec.outcome.is_terminal = true;
    rec.prediction = 3.2;
    rec.prediction_error = 1.8;
    rec.world_context = "meadow_forage";

    uint64_t hash1 = rec.compute_hash();

    // JSON roundtrip
    nlohmann::json j = rec.to_json();
    flgod::ExperienceRecord restored;
    restored.from_json(j);

    if (restored.compute_hash() != hash1) {
        std::cerr << "FAILED: ExperienceRecord JSON roundtrip hash mismatch!" << std::endl;
        return 1;
    }
    if (restored.agent_id != 42 || restored.action.discrete_action != 3 || restored.reward != 5.0) {
        std::cerr << "FAILED: Restored ExperienceRecord data mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  ExperienceRecord JSON roundtrip verified." << std::endl;

    // ReplayBuffer test
    flgod::ReplayBuffer buffer(100);
    for (uint32_t i = 0; i < 150; ++i) {
        flgod::ExperienceRecord item;
        item.agent_id = i;
        item.observation.state_id = i % 10;
        item.action.discrete_action = i % 4;
        item.reward = (i % 5 == 0) ? 1.0 : 0.0;
        buffer.push(item);
    }

    if (buffer.size() != 100) {
        std::cerr << "FAILED: ReplayBuffer capacity not maintained: size = " << buffer.size() << std::endl;
        return 1;
    }
    // Oldest items (0-49) should have been evicted; first item should have agent_id 50
    if (buffer[0].agent_id != 50) {
        std::cerr << "FAILED: Buffer FIFO eviction error: buffer[0].agent_id = " << buffer[0].agent_id << std::endl;
        return 1;
    }

    // Deterministic sampling
    flgod::Xoshiro256PlusPlus rng1(123456789ULL);
    flgod::Xoshiro256PlusPlus rng2(123456789ULL);

    auto batch1 = buffer.sample(16, rng1);
    auto batch2 = buffer.sample(16, rng2);

    if (batch1.size() != 16 || batch2.size() != 16) {
        std::cerr << "FAILED: Sample batch size incorrect!" << std::endl;
        return 1;
    }

    for (size_t i = 0; i < 16; ++i) {
        if (batch1[i].compute_hash() != batch2[i].compute_hash()) {
            std::cerr << "FAILED: ReplayBuffer deterministic sampling diverged!" << std::endl;
            return 1;
        }
    }
    std::cout << "  ReplayBuffer FIFO eviction and deterministic batch sampling verified." << std::endl;

    std::cout << "[TEST] test_learning_experience PASSED" << std::endl;
    return 0;
}
