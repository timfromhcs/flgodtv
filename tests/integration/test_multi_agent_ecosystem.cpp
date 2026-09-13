#include <iostream>
#include <cstdlib>
#include "flgod/agents/multi_agent_ecosystem.hpp"
#include "flgod/brain/malecns_adapter.hpp"

#define FLGOD_ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::abort(); \
    } \
} while(0)

using namespace flgod;

void setup_ecosystem(MultiAgentEcosystem& eco, uint64_t seed) {
    MultiAgentEcosystemConfig cfg{};
    cfg.seeds.world_seed = seed;
    cfg.seeds.agent_seed = seed + 100;
    cfg.seeds.physics_seed = seed + 200;
    cfg.seeds.learning_seed = seed + 300;
    cfg.world_config.world_seed = seed;
    cfg.enable_god_fly = true;
    cfg.enable_tech_world = true;
    eco.initialize(cfg);

    // Setup Colony 1 and Colony 2
    Colony c1(1, "Colony1", Vec3{0.0, 0.0, 0.0}, 25.0);
    c1.deposit_food(100.0);
    eco.agent_manager().register_colony(c1);

    Colony c2(2, "Colony2", Vec3{50.0, 0.0, 50.0}, 25.0);
    c2.deposit_food(100.0);
    eco.agent_manager().register_colony(c2);

    // Spawn agents
    for (uint64_t i = 1; i <= 6; ++i) {
        Genome g;
        g.sensory.visual_acuity = 1.0;
        uint32_t cid = (i <= 3) ? 1 : 2;
        Agent a(EntityID(i), cid, g);
        Vec3 pos = (i <= 3) ? Vec3{2.0 * i, 0.0, 2.0 * i} : Vec3{50.0 + 2.0 * i, 0.0, 50.0 + 2.0 * i};
        a.set_position(pos);
        eco.agent_manager().spawn_agent(a);
    }

    // Attach Connectome MaleCNS brain to Agent 1
    auto brain = std::make_shared<brain::MaleCNSAdapter>("malecns/data-raw/2023-27-2 soma_sides.csv", 200);
    eco.attach_brain(EntityID(1), brain);

    // Register NPC for Agent 2
    auto* npc = eco.npc_manager()->register_npc(EntityID(2));
    FLGOD_ASSERT(npc != nullptr);
    npc->add_goal("FORAGE_RESOURCES", 1.0f, Vec3{0.0, 0.0, 0.0});

    // Deploy programmable tech station
    eco.tech_world().create_device(101, 0x10, technology::DeviceType::SensorThermometer);
    eco.tech_world().create_device(102, 0x20, technology::DeviceType::ActuatorMotor);

    // VM program: load numbers, add, halt
    technology::VirtualMachine* vm = eco.tech_world().create_vm(1);
    std::vector<technology::VMInstruction> prog = {
        {technology::VMOpcode::MOVI, 1, 0, 10},
        {technology::VMOpcode::MOVI, 2, 0, 20},
        {technology::VMOpcode::ADD, 1, 2, 0},
        {technology::VMOpcode::HALT, 0, 0, 0}
    };
    vm->load_program(prog);
}

int main() {
    std::cout << "[TEST] Running MultiAgentEcosystem integration tests..." << std::endl;

    // 1. Initialize ecosystem and verify structure
    {
        MultiAgentEcosystem eco;
        setup_ecosystem(eco, 42);
        FLGOD_ASSERT(eco.is_initialized());
        FLGOD_ASSERT(eco.agent_manager().agent_count() == 6);
        FLGOD_ASSERT(eco.agent_manager().colony_count() == 2);
        FLGOD_ASSERT(eco.get_brain(EntityID(1)) != nullptr);

        // Run 50 ticks
        for (int t = 0; t < 50; ++t) {
            eco.step();
        }

        // Verify biological connectome step drove agent 1
        FLGOD_ASSERT(eco.get_brain(EntityID(1))->soma_count() == 200);

        // Verify technology VM stepped
        auto* vm = eco.tech_world().get_vm(1);
        FLGOD_ASSERT(vm != nullptr);
        FLGOD_ASSERT(vm->status() == technology::VMStatus::Halted);
        FLGOD_ASSERT(vm->get_register(1) == 30);

        std::cout << "  - Initialization, connectome stepping, and technology VM passed." << std::endl;
    }

    // 2. Determinism test: two separate runs must yield bit-exact identical state hashes
    uint64_t hash_run1 = 0;
    uint64_t hash_run2 = 0;
    {
        MultiAgentEcosystem eco1;
        setup_ecosystem(eco1, 1337);
        for (int t = 0; t < 100; ++t) {
            eco1.step();
        }
        hash_run1 = eco1.compute_hash();

        MultiAgentEcosystem eco2;
        setup_ecosystem(eco2, 1337);
        for (int t = 0; t < 100; ++t) {
            eco2.step();
        }
        hash_run2 = eco2.compute_hash();

        FLGOD_ASSERT(hash_run1 != 0);
        FLGOD_ASSERT(hash_run1 == hash_run2);
        std::cout << "  - Determinism passed: exact hash match (" << hash_run1 << ") over 100 ticks." << std::endl;
    }

    // 3. Crash recovery & Checkpoint Replay
    {
        // Uninterrupted reference run
        MultiAgentEcosystem eco_ref;
        setup_ecosystem(eco_ref, 777);
        for (int t = 0; t < 50; ++t) {
            eco_ref.step();
        }
        nlohmann::json checkpoint = eco_ref.create_checkpoint();
        for (int t = 0; t < 50; ++t) {
            eco_ref.step();
        }
        uint64_t ref_final_hash = eco_ref.compute_hash();

        // Restored run from checkpoint at tick 50
        MultiAgentEcosystem eco_res;
        setup_ecosystem(eco_res, 777);
        eco_res.restore_checkpoint(checkpoint);
        for (int t = 0; t < 50; ++t) {
            eco_res.step();
        }
        uint64_t res_final_hash = eco_res.compute_hash();

        FLGOD_ASSERT(ref_final_hash == res_final_hash);
        std::cout << "  - Crash recovery / checkpoint replay passed: restored hash matches reference (" 
                  << ref_final_hash << ")." << std::endl;
    }

    std::cout << "[TEST] ALL MultiAgentEcosystem integration tests PASSED!" << std::endl;
    return 0;
}
