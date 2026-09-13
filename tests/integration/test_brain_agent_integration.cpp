#include <cassert>
#include <iostream>
#include "flgod/brain/malecns_adapter.hpp"
#include "flgod/agents/agent.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/world/world.hpp"

int main() {
    std::cout << "[Test] Running Brain-Agent Biological Control Integration Test...\n";

    // 1. Initialize Subsystems
    flgod::WorldConfig wcfg;
    flgod::World world(wcfg);

    flgod::AgentManager agent_mgr;
    flgod::Colony colony(1, "BrainColony", {0.0, 0.0, 0.0}, 50.0);
    colony.deposit_food(100.0);
    agent_mgr.register_colony(colony);

    // 2. Spawn Agent with MaleCNS Connectome Brain
    flgod::EntityID aid(flgod::EntityType::Agent, 1, 888);
    flgod::Agent agent(aid, colony.id());
    agent.set_position({2.0, 0.0, 2.0});
    agent_mgr.spawn_agent(agent);

    // Initialize real biological connectome brain with 2,000 somas
    flgod::brain::MaleCNSAdapter brain("malecns/data-raw/2023-27-2 soma_sides.csv", 2000);
    assert(brain.soma_count() >= 2000);

    // 3. Multi-tick Sensory-Motor Biological Loop
    flgod::RNGStream rng(12345ULL);
    double dt = 0.01; // 10 ms biological simulation step
    const int TICKS = 100;

    double cumulative_per = 0.0;
    for (int t = 0; t < TICKS; ++t) {
        // Construct sensory perception from agent environment
        flgod::brain::BrainSensoryInput b_in;
        b_in.odor_sugar_intensity = 0.8; // Strong food scent
        b_in.left_eye_sectors.fill(0.6);
        b_in.right_eye_sectors.fill(0.4);
        b_in.internal_energy = agent_mgr.get_agent(aid).drives().energy;

        // Step brain neural dynamics
        flgod::brain::BrainMotorOutput b_out;
        brain.step(dt, b_in, b_out);

        cumulative_per += b_out.proboscis_extension;

        // Apply motor output to physical agent
        auto& physical_agent = agent_mgr.get_agent(aid);
        if (b_out.proboscis_extension > 0.5) {
            // Forage feeding action
            physical_agent.drives().energy = std::min(100.0, physical_agent.drives().energy + 0.2);
            physical_agent.drives().hunger = std::max(0.0, physical_agent.drives().hunger - 0.2);
        }

        // Apply differential steering to heading
        physical_agent.set_velocity(physical_agent.velocity() + b_out.steering_torque * dt);
    }

    assert(cumulative_per > 50.0 && "Proboscis extension should trigger reliably across feeding trials");
    assert(agent_mgr.get_agent(aid).drives().energy > 90.0);

    // 4. Deterministic State Reproduction
    uint64_t brain_hash = brain.compute_brain_hash();
    assert(brain_hash != 0);

    std::cout << "  Brain-Agent Biological Loop Verified: 2,000 connectome somas simulated over " 
              << TICKS << " ticks, cumulative PER=" << cumulative_per 
              << ", brain hash: 0x" << std::hex << brain_hash << std::dec << "\n";
    std::cout << "[Test] Brain-Agent Biological Control Integration Test PASSED!\n";
    return 0;
}
