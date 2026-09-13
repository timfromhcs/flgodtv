#include "flgod/agents/agent_manager.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_multi_agent_simulation..." << std::endl;

    auto setup_simulation = [](flgod::AgentManager& mgr, flgod::World& world) {
        // Register 2 colonies
        flgod::Colony c1(1, "Colony_Alpha", flgod::Vec3(15.0, 0.0, 15.0), 25.0);
        c1.deposit_food(100.0);
        flgod::Colony c2(2, "Colony_Beta", flgod::Vec3(-15.0, 0.0, -15.0), 25.0);
        c2.deposit_food(100.0);

        mgr.register_colony(c1);
        mgr.register_colony(c2);

        // Spawn 10 agents (5 in Alpha, 5 in Beta)
        for (uint64_t i = 1; i <= 10; ++i) {
            uint32_t cid = (i <= 5) ? 1 : 2;
            flgod::EntityID aid(flgod::EntityType::Agent, 1, i);
            flgod::Agent a(aid, cid);
            flgod::Vec3 base = (cid == 1) ? c1.nest_position() : c2.nest_position();
            a.set_position(base + flgod::Vec3(i * 0.5, 0.0, i * 0.5));
            mgr.spawn_agent(a);
        }
    };

    // 1. Bit-exact determinism test
    flgod::AgentManager mgr1, mgr2;
    flgod::World world1, world2;
    setup_simulation(mgr1, world1);
    setup_simulation(mgr2, world2);

    flgod::RNGStream rng1(991188ULL);
    flgod::RNGStream rng2(991188ULL);

    double dt = 0.05;
    const int total_ticks = 200;

    for (int t = 0; t < total_ticks; ++t) {
        mgr1.step(dt, world1, rng1);
        mgr2.step(dt, world2, rng2);

        uint64_t h1 = mgr1.compute_agents_hash();
        uint64_t h2 = mgr2.compute_agents_hash();

        if (h1 != h2) {
            std::cerr << "FAILED: Multi-agent determinism divergence at tick " << t
                      << ": h1=0x" << std::hex << h1 << " != h2=0x" << h2 << std::dec << std::endl;
            return 1;
        }
    }
    std::cout << "  Passed 200 ticks multi-agent bit-exact determinism check. Hash: 0x" 
              << std::hex << mgr1.compute_agents_hash() << std::dec << std::endl;

    // 2. Checkpoint and crash recovery test
    flgod::AgentManager ref_mgr;
    flgod::World ref_world;
    setup_simulation(ref_mgr, ref_world);
    flgod::RNGStream ref_rng(991188ULL);

    for (int t = 0; t < total_ticks; ++t) {
        ref_mgr.step(dt, ref_world, ref_rng);
    }
    uint64_t expected_final_hash = ref_mgr.compute_agents_hash();

    flgod::AgentManager inter_mgr;
    flgod::World inter_world;
    setup_simulation(inter_mgr, inter_world);
    flgod::RNGStream inter_rng(991188ULL);

    for (int t = 0; t < 100; ++t) {
        inter_mgr.step(dt, inter_world, inter_rng);
    }

    // Serialize checkpoint
    nlohmann::json cp = inter_mgr.to_json();
    std::string cp_str = cp.dump();

    // Restore into fresh agent manager
    flgod::AgentManager rec_mgr;
    rec_mgr.from_json(nlohmann::json::parse(cp_str));

    if (rec_mgr.agent_count() != inter_mgr.agent_count()) {
        std::cerr << "FAILED: Restored agent count mismatch!" << std::endl;
        return 1;
    }

    // Continue remaining 100 ticks
    for (int t = 100; t < total_ticks; ++t) {
        rec_mgr.step(dt, inter_world, inter_rng);
    }

    uint64_t recovered_final_hash = rec_mgr.compute_agents_hash();
    if (recovered_final_hash != expected_final_hash) {
        std::cerr << "FAILED: Restored multi-agent run diverged! Expected: 0x"
                  << std::hex << expected_final_hash << ", Got: 0x" << recovered_final_hash << std::dec << std::endl;
        return 1;
    }
    std::cout << "  Passed 100% crash recovery test at tick 100 -> 200. Final hash: 0x"
              << std::hex << recovered_final_hash << std::dec << std::endl;

    std::cout << "[TEST] test_multi_agent_simulation PASSED" << std::endl;
    return 0;
}
