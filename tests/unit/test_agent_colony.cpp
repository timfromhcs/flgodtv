#include "flgod/agents/colony.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_agent_colony..." << std::endl;

    flgod::Colony colony(1, "Northern_Hive", flgod::Vec3(10.0, 0.0, 10.0), 30.0);

    flgod::EntityID a1(flgod::EntityType::Agent, 1, 201);
    flgod::EntityID a2(flgod::EntityType::Agent, 1, 202);

    colony.add_member(a1);
    colony.add_member(a2);

    if (colony.population_count() != 2) {
        std::cerr << "FAILED: Colony population count mismatch: " << colony.population_count() << std::endl;
        return 1;
    }

    // Collective food deposit and withdrawal
    colony.deposit_food(50.0);
    if (colony.stored_resources() != 50.0) {
        std::cerr << "FAILED: Colony deposit failed: " << colony.stored_resources() << std::endl;
        return 1;
    }

    double withdrawn = colony.withdraw_food(20.0);
    if (withdrawn != 20.0 || colony.stored_resources() != 30.0) {
        std::cerr << "FAILED: Colony withdrawal failed: withdrawn=" << withdrawn 
                  << ", remaining=" << colony.stored_resources() << std::endl;
        return 1;
    }
    std::cout << "  Colony collective resource sharing verified: stored=" << colony.stored_resources() << std::endl;

    // Serialization test
    nlohmann::json j = colony.to_json();
    flgod::Colony restored;
    restored.from_json(j);

    if (restored.compute_hash() != colony.compute_hash()) {
        std::cerr << "FAILED: Colony serialization hash mismatch!" << std::endl;
        return 1;
    }
    if (restored.population_count() != 2 || restored.stored_resources() != 30.0) {
        std::cerr << "FAILED: Restored colony fields mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  Colony serialization roundtrip verified." << std::endl;

    std::cout << "[TEST] test_agent_colony PASSED" << std::endl;
    return 0;
}
