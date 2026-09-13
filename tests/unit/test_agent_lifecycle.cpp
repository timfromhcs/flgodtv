#include "flgod/agents/agent.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_agent_lifecycle..." << std::endl;

    flgod::EntityID agent_id(flgod::EntityType::Agent, 1, 101);
    flgod::Agent agent(agent_id);
    agent.set_position(flgod::Vec3(0.0, 0.0, 0.0));

    flgod::RNGStream rng(12345ULL);
    double dt = 0.1;

    // 1. Natural metabolic drain over 100 ticks without food
    for (int t = 0; t < 100; ++t) {
        flgod::AgentSensoryInput input{}; // No food detected
        agent.step(dt, input, rng);
    }

    if (agent.drives().energy >= 100.0) {
        std::cerr << "FAILED: Energy did not decrease after 100 ticks: " << agent.drives().energy << std::endl;
        return 1;
    }
    if (agent.drives().hunger <= 0.0) {
        std::cerr << "FAILED: Hunger did not increase after 100 ticks: " << agent.drives().hunger << std::endl;
        return 1;
    }
    std::cout << "  Metabolic drain verified: energy=" << agent.drives().energy 
              << ", hunger=" << agent.drives().hunger << std::endl;

    // 2. Foraging response when food is detected close by
    agent.drives().hunger = 60.0;
    flgod::AgentSensoryInput food_input{};
    food_input.food_detected = true;
    food_input.food_distance = 0.5; // close enough to forage
    food_input.food_direction = flgod::Vec3(0.5, 0.0, 0.0);

    double energy_before = agent.drives().energy;
    auto out = agent.step(dt, food_input, rng);

    if (out.action != flgod::AgentActionType::Forage) {
        std::cerr << "FAILED: Agent did not forage when hungry and near food! Action=" 
                  << static_cast<int>(out.action) << std::endl;
        return 1;
    }
    if (agent.drives().energy <= energy_before) {
        std::cerr << "FAILED: Energy did not increase after foraging!" << std::endl;
        return 1;
    }
    std::cout << "  Foraging response verified: energy replenished to " << agent.drives().energy << std::endl;

    // 3. Serialization roundtrip
    nlohmann::json j = agent.to_json();
    flgod::Agent restored;
    restored.from_json(j);

    if (restored.compute_hash() != agent.compute_hash()) {
        std::cerr << "FAILED: Agent serialization hash mismatch!" << std::endl;
        return 1;
    }
    std::cout << "  Agent serialization roundtrip verified." << std::endl;

    std::cout << "[TEST] test_agent_lifecycle PASSED" << std::endl;
    return 0;
}
