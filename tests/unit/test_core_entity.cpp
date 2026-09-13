#include "flgod/core/entity_id.hpp"
#include <iostream>
#include <unordered_set>

int main() {
    std::cout << "[TEST] Running test_core_entity..." << std::endl;

    // 1. Basic properties
    flgod::EntityID null_id;
    if (null_id.is_valid()) {
        std::cerr << "FAILED: Default EntityID should be invalid" << std::endl;
        return 1;
    }

    flgod::EntityID id1(flgod::EntityType::Agent, 3, 42);
    if (!id1.is_valid()) {
        std::cerr << "FAILED: Constructed EntityID should be valid" << std::endl;
        return 1;
    }
    if (id1.type() != flgod::EntityType::Agent) {
        std::cerr << "FAILED: Expected Agent type, got " << static_cast<int>(id1.type()) << std::endl;
        return 1;
    }
    if (id1.generation() != 3) {
        std::cerr << "FAILED: Expected generation 3, got " << id1.generation() << std::endl;
        return 1;
    }
    if (id1.index() != 42) {
        std::cerr << "FAILED: Expected index 42, got " << id1.index() << std::endl;
        return 1;
    }

    // 2. Allocator test
    flgod::EntityIDAllocator allocator(1);
    std::unordered_set<flgod::EntityID> unique_ids;

    const int n_alloc = 50000;
    for (int i = 0; i < n_alloc; ++i) {
        flgod::EntityType type = (i % 2 == 0) ? flgod::EntityType::Agent : flgod::EntityType::WorldObject;
        flgod::EntityID eid = allocator.allocate(type);
        if (!eid.is_valid()) {
            std::cerr << "FAILED: Allocator produced invalid EntityID at step " << i << std::endl;
            return 1;
        }
        if (!unique_ids.insert(eid).second) {
            std::cerr << "FAILED: Collision detected for allocated EntityID at step " << i << std::endl;
            return 1;
        }
    }

    if (unique_ids.size() != n_alloc) {
        std::cerr << "FAILED: Set size does not match allocation count!" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_core_entity passed successfully." << std::endl;
    return 0;
}
