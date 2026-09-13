#include "flgod/core/simulation.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

void print_usage(const char* prog) {
    std::cout << "FLGODTV Headless Simulation Platform v" << flgod::CURRENT_SIMULATION_VERSION.to_string() << "\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Modes:\n"
              << "  --headless              Run simulation in headless mode\n"
              << "  --self-test             Execute complete built-in self-tests\n"
              << "  --benchmark             Run performance and throughput benchmark\n"
              << "  --simulate [ticks]      Run simulation for specified number of ticks (default: 600)\n"
              << "  --validate              Validate simulation state consistency\n"
              << "  --version               Display version and build information\n"
              << "  --help                  Show this help text\n";
}

int run_self_test() {
    std::cout << "=== FLGODTV CORE SELF-TEST ===" << std::endl;
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "self_test_run";
    flgod::Simulation sim;
    sim.initialize(cfg);

    std::cout << "[1/4] Checking initialization... ";
    if (!sim.is_initialized()) {
        std::cout << "FAILED" << std::endl;
        return 1;
    }
    std::cout << "OK" << std::endl;

    std::cout << "[2/4] Stepping simulation for 120 ticks... ";
    for (int i = 0; i < 120; ++i) {
        sim.step();
    }
    if (sim.state().clock().tick() != 120) {
        std::cout << "FAILED (tick != 120)" << std::endl;
        return 1;
    }
    std::cout << "OK (tick=" << sim.state().clock().tick() << ")" << std::endl;

    std::cout << "[3/4] Checkpointing state... ";
    nlohmann::json cp = sim.create_checkpoint();
    uint64_t hash_before = sim.compute_state_hash();
    std::cout << "OK (hash=0x" << std::hex << hash_before << std::dec << ")" << std::endl;

    std::cout << "[4/4] Restoring state and validating... ";
    flgod::Simulation sim2;
    sim2.initialize(cfg);
    sim2.restore_checkpoint(cp);
    uint64_t hash_after = sim2.compute_state_hash();
    if (hash_before != hash_after) {
        std::cout << "FAILED (hash mismatch: " << hash_before << " != " << hash_after << ")" << std::endl;
        return 1;
    }
    std::cout << "OK (hash verified identical)" << std::endl;

    std::cout << "=== ALL CORE SELF-TESTS PASSED ===" << std::endl;
    return 0;
}

int run_benchmark() {
    std::cout << "=== FLGODTV CORE BENCHMARK ===" << std::endl;
    const uint64_t benchmark_ticks = 100000;
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "benchmark_run";
    flgod::Simulation sim;
    sim.initialize(cfg);

    auto start_time = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < benchmark_ticks; ++i) {
        sim.state().rng().agent().next_double();
        sim.step();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end_time - start_time;
    double seconds = duration.count();
    double ticks_per_sec = benchmark_ticks / seconds;

    std::cout << "Executed " << benchmark_ticks << " ticks in " << seconds << " seconds.\n"
              << "Throughput: " << static_cast<uint64_t>(ticks_per_sec) << " ticks/sec ("
              << (ticks_per_sec / 60.0) << "x realtime at 60 Hz)\n"
              << "State Hash: 0x" << std::hex << sim.compute_state_hash() << std::dec << "\n"
              << "=== BENCHMARK COMPLETE ===" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        print_usage(argv[0]);
        return 0;
    }

    std::string mode = argv[1];

    if (mode == "--version" || mode == "-v") {
        std::cout << "FLGODTV v" << flgod::CURRENT_SIMULATION_VERSION.to_string()
                  << " (Schema v" << flgod::CURRENT_SIMULATION_VERSION.schema_version << ")\n";
        return 0;
    }

    if (mode == "--help" || mode == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    if (mode == "--self-test") {
        return run_self_test();
    }

    if (mode == "--benchmark") {
        return run_benchmark();
    }

    if (mode == "--headless" || mode == "--simulate") {
        uint64_t ticks = 600;
        if (argc > 2) {
            ticks = std::stoull(argv[2]);
        }
        std::cout << "[FLGODTV] Running headless simulation for " << ticks << " ticks...\n";
        flgod::SimulationConfig cfg;
        flgod::Simulation sim;
        sim.initialize(cfg);
        sim.run_ticks(ticks);
        std::cout << "[FLGODTV] Completed " << ticks << " ticks. Final state hash: 0x"
                  << std::hex << sim.compute_state_hash() << std::dec << "\n";
        return 0;
    }

    if (mode == "--validate") {
        return run_self_test();
    }

    std::cerr << "Unknown mode: " << mode << "\n";
    print_usage(argv[0]);
    return 1;
}
