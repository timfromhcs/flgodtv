#include "flgod/learning/memory.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_learning_memory..." << std::endl;

    flgod::MemorySystem mem;

    // 1. Working memory capacity limit (7 items)
    for (int i = 0; i < 10; ++i) {
        mem.working().add_item("focus_" + std::to_string(i), {static_cast<double>(i)});
    }
    if (mem.working().size() != 7) {
        std::cerr << "FAILED: Working memory capacity exceeded 7: size = " << mem.working().size() << std::endl;
        return 1;
    }
    std::cout << "  Working memory capacity (7 items) verified." << std::endl;

    // 2. Episodic memory
    mem.episodic().record_event(1, 0.1, 2, 1, 1.0);  // High reward
    mem.episodic().record_event(1, 0.2, 3, 0, -0.8); // High penalty
    mem.episodic().record_event(1, 0.3, 2, 1, 0.9);

    auto matches = mem.episodic().query_by_state(2);
    if (matches.size() != 2) {
        std::cerr << "FAILED: Episodic memory query failed: expected 2 matches, got " << matches.size() << std::endl;
        return 1;
    }
    std::cout << "  Episodic memory recording and state query verified." << std::endl;

    // 3. Social memory
    mem.social().update_interaction(101, 0.8, 1.0);  // Friendly interaction
    mem.social().update_interaction(102, -0.9, 1.0); // Hostile interaction
    if (mem.social().get_trust(101) <= 0.0 || mem.social().get_trust(102) >= 0.0) {
        std::cerr << "FAILED: Social memory trust scores incorrect!" << std::endl;
        return 1;
    }
    std::cout << "  Social memory trust score tracking verified." << std::endl;

    // 4. Memory consolidation
    mem.consolidate();

    // High reward event (state 2, act 1) should reinforce procedural habit
    double habit_strength = mem.procedural().get_habit_strength(2, 1);
    if (habit_strength <= 0.0) {
        std::cerr << "FAILED: Memory consolidation failed to reinforce procedural habit!" << std::endl;
        return 1;
    }

    // High reward event should create semantic fact
    if (!mem.semantic().has_fact("state_2_act_1")) {
        std::cerr << "FAILED: Memory consolidation failed to create semantic fact!" << std::endl;
        return 1;
    }
    std::cout << "  Memory consolidation from episodic to procedural & semantic verified." << std::endl;

    // 5. Full memory serialization roundtrip
    nlohmann::json j = mem.to_json();
    flgod::MemorySystem restored;
    restored.from_json(j);

    if (restored.working().size() != mem.working().size() ||
        restored.episodic().size() != mem.episodic().size() ||
        restored.semantic().fact_count() != mem.semantic().fact_count() ||
        restored.social().peer_count() != mem.social().peer_count()) {
        std::cerr << "FAILED: MemorySystem JSON restoration size mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  Multi-tier memory serialization roundtrip verified." << std::endl;

    std::cout << "[TEST] test_learning_memory PASSED" << std::endl;
    return 0;
}
