#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "flgod/agents/multi_agent_ecosystem.hpp"
#include "flgod/brain/malecns_adapter.hpp"

using namespace flgod;

int main() {
    std::cout << "========================================================" << std::endl;
    std::cout << "  FLGODTV MULTI-AGENT ECOSYSTEM BENCHMARK" << std::endl;
    std::cout << "========================================================" << std::endl;

    MultiAgentEcosystem eco;
    MultiAgentEcosystemConfig cfg{};
    cfg.seeds.world_seed = 999;
    cfg.seeds.agent_seed = 1000;
    cfg.world_config.world_seed = 999;
    eco.initialize(cfg);

    // Register 4 Colonies
    for (uint32_t c = 1; c <= 4; ++c) {
        Vec3 nest{static_cast<double>(c * 30), 0.0, static_cast<double>(c * 30)};
        std::string name = "Colony" + std::to_string(c);
        Colony col(c, name, nest, 20.0);
        col.deposit_food(200.0);
        eco.agent_manager().register_colony(col);
    }

    // Spawn 20 agents (5 per colony)
    for (uint64_t i = 1; i <= 20; ++i) {
        uint32_t cid = static_cast<uint32_t>((i - 1) / 5) + 1;
        Genome g;
        Agent a(EntityID(i), cid, g);
        Vec3 pos{static_cast<double>(cid * 30 + (i % 5)), 0.0, static_cast<double>(cid * 30 + (i % 5))};
        a.set_position(pos);
        eco.agent_manager().spawn_agent(a);

        // Attach connectome MaleCNS adapter to first 4 agents
        if (i <= 4) {
            auto brain = std::make_shared<brain::MaleCNSAdapter>();
            brain->load_from_csv("malecns/data-raw/2023-27-2 soma_sides.csv", 1000);
            eco.attach_brain(EntityID(i), brain);
        }
    }

    // Deploy programmable tech stations
    for (uint32_t s = 1; s <= 2; ++s) {
        eco.tech_world().create_device(200 + s, static_cast<uint16_t>(0x10 + s), technology::DeviceType::SensorThermometer);
        eco.tech_world().create_device(300 + s, static_cast<uint16_t>(0x20 + s), technology::DeviceType::ActuatorMotor);
    }

    // Warm-up
    for (int t = 0; t < 20; ++t) {
        eco.step();
    }

    // Benchmark 500 ticks
    const uint64_t benchmark_ticks = 500;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint64_t t = 0; t < benchmark_ticks; ++t) {
        eco.step();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    double duration_sec = duration_ms / 1000.0;
    double ticks_per_sec = static_cast<double>(benchmark_ticks) / duration_sec;
    double realtime_factor = ticks_per_sec / 60.0;

    std::cout << "Simulated Ticks:   " << benchmark_ticks << " in " << duration_ms << " ms" << std::endl;
    std::cout << "Simulation Rate:   " << ticks_per_sec << " ticks/sec (" << realtime_factor << "x realtime at 60Hz)" << std::endl;
    std::cout << "Agents Active:     " << eco.agent_manager().agent_count() << std::endl;
    std::cout << "Colonies:          " << eco.agent_manager().colony_count() << std::endl;
    std::cout << "Final Hash:        " << eco.compute_hash() << std::endl;

    // Save evidence
    std::filesystem::create_directories("evidence/windows");
    nlohmann::json report;
    report["benchmark_ticks"] = benchmark_ticks;
    report["duration_ms"] = duration_ms;
    report["ticks_per_second"] = ticks_per_sec;
    report["realtime_factor_60hz"] = realtime_factor;
    report["agent_count"] = eco.agent_manager().agent_count();
    report["colony_count"] = eco.agent_manager().colony_count();
    report["final_hash"] = eco.compute_hash();

    std::ofstream out_file("evidence/windows/multi_agent_benchmark.json");
    if (out_file.is_open()) {
        out_file << report.dump(2) << std::endl;
        std::cout << "Saved benchmark evidence to: evidence/windows/multi_agent_benchmark.json" << std::endl;
    }
    std::cout << "========================================================" << std::endl;

    return 0;
}
