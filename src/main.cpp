#include "flgod/core/simulation.hpp"
#include "flgod/core/replay.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>

void print_usage(const char* prog) {
    std::cout << "FLGODTV Headless Simulation Platform v" << flgod::CURRENT_SIMULATION_VERSION.to_string() << "\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Execution Modes (GEMINI.md Sections 60 & 61):\n"
              << "  --headless              Run simulation in headless mode\n"
              << "  --self-test             Execute complete built-in self-tests\n"
              << "  --benchmark [ticks]     Run throughput benchmark (default: 100000 ticks)\n"
              << "  --simulate [ticks]      Run simulation for specified ticks (default: 600)\n"
              << "  --train [episodes]      Run headless learning & experience training loop\n"
              << "  --evolve [generations]  Run headless evolutionary population stepping\n"
              << "  --validate              Validate simulation state consistency and invariants\n"
              << "  --checkpoint <path>     Run simulation and save checkpoint snapshot to file\n"
              << "  --restore <path>        Restore simulation from checkpoint file and continue\n"
              << "  --replay <path>         Verify bit-exact replay from recorded replay log\n"
              << "  --seed <uint64>         Set primary world seed (default: 133701)\n"
              << "  --version               Display version, schema, and build metadata\n"
              << "  --help                  Show this help text\n";
}

int run_self_test() {
    std::cout << "=== FLGODTV CORE SELF-TEST ===" << std::endl;
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "self_test_run";
    flgod::Simulation sim;
    sim.initialize(cfg);

    std::cout << "[1/5] Checking initialization... ";
    if (!sim.is_initialized()) {
        std::cout << "FAILED" << std::endl;
        return 1;
    }
    std::cout << "OK" << std::endl;

    std::cout << "[2/5] Stepping simulation for 120 ticks... ";
    for (int i = 0; i < 120; ++i) {
        sim.step();
    }
    if (sim.state().clock().tick() != 120) {
        std::cout << "FAILED (tick != 120)" << std::endl;
        return 1;
    }
    std::cout << "OK (tick=" << sim.state().clock().tick() << ")" << std::endl;

    std::cout << "[3/5] Checkpointing state... ";
    nlohmann::json cp = sim.create_checkpoint();
    uint64_t hash_before = sim.compute_state_hash();
    std::cout << "OK (hash=0x" << std::hex << hash_before << std::dec << ")" << std::endl;

    std::cout << "[4/5] Restoring state and validating... ";
    flgod::Simulation sim2;
    sim2.initialize(cfg);
    sim2.restore_checkpoint(cp);
    uint64_t hash_after = sim2.compute_state_hash();
    if (hash_before != hash_after) {
        std::cout << "FAILED (hash mismatch: 0x" << std::hex << hash_before << " != 0x" << hash_after << std::dec << ")" << std::endl;
        return 1;
    }
    std::cout << "OK (hash verified identical)" << std::endl;

    std::cout << "[5/5] Testing replay recording and verification... ";
    flgod::Simulation sim3;
    sim3.initialize(cfg);
    flgod::ReplayLog log = flgod::ReplayManager::record(sim3, 100, 25);
    std::string replay_err;
    if (!flgod::ReplayManager::verify(log, &replay_err)) {
        std::cout << "FAILED: " << replay_err << std::endl;
        return 1;
    }
    std::cout << "OK (replay verified bit-exact)" << std::endl;

    std::cout << "=== ALL CORE SELF-TESTS PASSED ===" << std::endl;
    return 0;
}

int run_benchmark(uint64_t benchmark_ticks) {
    std::cout << "=== FLGODTV CORE BENCHMARK ===" << std::endl;
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "benchmark_run";
    flgod::Simulation sim;
    sim.initialize(cfg);

    auto start_time = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < benchmark_ticks; ++i) {
        sim.step();
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end_time - start_time;
    double seconds = duration.count();
    double ticks_per_sec = (seconds > 0.0) ? (benchmark_ticks / seconds) : 0.0;

    std::cout << "Executed " << benchmark_ticks << " ticks in " << seconds << " seconds.\n"
              << "Throughput: " << static_cast<uint64_t>(ticks_per_sec) << " ticks/sec ("
              << (ticks_per_sec / 60.0) << "x realtime at 60 Hz)\n"
              << "State Hash: 0x" << std::hex << sim.compute_state_hash() << std::dec << "\n"
              << "=== BENCHMARK COMPLETE ===" << std::endl;
    return 0;
}

int run_validate() {
    std::cout << "=== FLGODTV SIMULATION VALIDATION ===" << std::endl;
    flgod::SimulationConfig cfg;
    flgod::Simulation sim;
    sim.initialize(cfg);

    // 1. Clock invariants
    std::cout << "Checking clock invariants... ";
    uint64_t prev_tick = sim.state().clock().tick();
    double prev_time = sim.state().clock().elapsed_time();
    for (int i = 0; i < 60; ++i) {
        sim.step();
        if (sim.state().clock().tick() <= prev_tick || sim.state().clock().elapsed_time() <= prev_time) {
            std::cout << "FAILED: Clock monotonicity violated!\n";
            return 1;
        }
        prev_tick = sim.state().clock().tick();
        prev_time = sim.state().clock().elapsed_time();
    }
    std::cout << "PASSED\n";

    // 2. Continuous world fields invariants
    std::cout << "Checking world fields bounded behavior... ";
    const auto& world = sim.state().world();
    for (double z = -100.0; z <= 100.0; z += 25.0) {
        for (double x = -100.0; x <= 100.0; x += 25.0) {
            double temp = world.sample_temperature(x, z);
            if (temp < -60.0 || temp > 60.0) {
                std::cout << "FAILED: Temperature out of bounds: " << temp << "\n";
                return 1;
            }
            double hum = world.sample_humidity(x, z);
            if (hum < 0.0 || hum > 1.0) {
                std::cout << "FAILED: Humidity out of bounds: " << hum << "\n";
                return 1;
            }
        }
    }
    std::cout << "PASSED\n";

    // 3. Physics engine invariants
    std::cout << "Checking physics determinism and stability... ";
    flgod::RigidBodyState test_body;
    test_body.id = flgod::EntityID(flgod::EntityType::WorldObject, 1, 999);
    test_body.position = flgod::Vec3(0.0, 5.0, 0.0);
    test_body.shape.radius = 0.5;
    sim.state().physics().add_body(test_body);
    for (int i = 0; i < 100; ++i) {
        sim.step();
    }
    if (sim.state().physics().get_body(test_body.id).position.y < 0.49) {
        std::cout << "FAILED: Body penetrated ground plane!\n";
        return 1;
    }
    std::cout << "PASSED\n";

    std::cout << "=== ALL VALIDATION CHECKS PASSED ===" << std::endl;
    return 0;
}

int run_train(uint64_t episodes) {
    std::cout << "=== FLGODTV HEADLESS TRAINING HARNESS ===" << std::endl;
    std::cout << "Starting headless training loop for " << episodes << " episodes...\n";
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "headless_train_session";
    flgod::Simulation sim;
    sim.initialize(cfg);

    uint64_t total_steps = 0;
    for (uint64_t ep = 1; ep <= episodes; ++ep) {
        // Run episode (e.g. 100 steps per episode)
        for (int step = 0; step < 100; ++step) {
            sim.step();
            total_steps++;
        }
        if (ep % 10 == 0 || ep == episodes) {
            std::cout << "  Episode " << ep << "/" << episodes 
                      << " completed (total steps: " << total_steps 
                      << ", hash: 0x" << std::hex << sim.compute_state_hash() << std::dec << ")\n";
        }
    }
    std::cout << "=== TRAINING COMPLETE (" << total_steps << " steps executed) ===" << std::endl;
    return 0;
}

int run_evolve(uint64_t generations) {
    std::cout << "=== FLGODTV HEADLESS EVOLUTION HARNESS ===" << std::endl;
    std::cout << "Starting headless evolutionary loop for " << generations << " generations...\n";
    flgod::SimulationConfig cfg;
    cfg.experiment_id = "headless_evolution_session";
    flgod::Simulation sim;
    sim.initialize(cfg);

    for (uint64_t gen = 1; gen <= generations; ++gen) {
        // Advance generation (e.g. 60 steps per generation)
        for (int step = 0; step < 60; ++step) {
            sim.step();
        }
        sim.state().id_allocator().advance_generation();
        if (gen % 5 == 0 || gen == generations) {
            std::cout << "  Generation " << gen << "/" << generations 
                      << " (allocator gen: " << sim.state().id_allocator().current_generation()
                      << ", hash: 0x" << std::hex << sim.compute_state_hash() << std::dec << ")\n";
        }
    }
    std::cout << "=== EVOLUTION COMPLETE ===" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        print_usage(argv[0]);
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    std::string primary_mode = args[0];

    if (primary_mode == "--version" || primary_mode == "-v") {
        std::cout << "FLGODTV v" << flgod::CURRENT_SIMULATION_VERSION.to_string()
                  << " (Schema v" << flgod::CURRENT_SIMULATION_VERSION.schema_version 
                  << ", Build: " << flgod::CURRENT_SIMULATION_VERSION.build_meta << ")\n";
        return 0;
    }

    if (primary_mode == "--help" || primary_mode == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    if (primary_mode == "--self-test") {
        return run_self_test();
    }

    if (primary_mode == "--validate") {
        return run_validate();
    }

    if (primary_mode == "--benchmark") {
        uint64_t ticks = 100000;
        if (args.size() > 1) {
            ticks = std::stoull(args[1]);
        }
        return run_benchmark(ticks);
    }

    if (primary_mode == "--train") {
        uint64_t episodes = 20;
        if (args.size() > 1) {
            episodes = std::stoull(args[1]);
        }
        return run_train(episodes);
    }

    if (primary_mode == "--evolve") {
        uint64_t generations = 10;
        if (args.size() > 1) {
            generations = std::stoull(args[1]);
        }
        return run_evolve(generations);
    }

    if (primary_mode == "--checkpoint") {
        if (args.size() < 2) {
            std::cerr << "Error: --checkpoint requires an output filepath\n";
            return 1;
        }
        std::string path = args[1];
        uint64_t ticks = 300;
        if (args.size() > 2) {
            ticks = std::stoull(args[2]);
        }
        flgod::SimulationConfig cfg;
        flgod::Simulation sim;
        sim.initialize(cfg);
        sim.run_ticks(ticks);

        nlohmann::json cp = sim.create_checkpoint();
        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "Error: Could not open " << path << " for writing\n";
            return 1;
        }
        out << cp.dump(2);
        out.close();

        std::cout << "[FLGODTV] Checkpoint successfully saved to " << path 
                  << " (tick: " << sim.state().clock().tick() 
                  << ", hash: 0x" << std::hex << sim.compute_state_hash() << std::dec << ")\n";
        return 0;
    }

    if (primary_mode == "--restore") {
        if (args.size() < 2) {
            std::cerr << "Error: --restore requires an input checkpoint filepath\n";
            return 1;
        }
        std::string path = args[1];
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "Error: Could not open " << path << " for reading\n";
            return 1;
        }
        nlohmann::json cp;
        in >> cp;
        in.close();

        flgod::SimulationConfig cfg;
        flgod::Simulation sim;
        sim.initialize(cfg);
        sim.restore_checkpoint(cp);

        uint64_t extra_ticks = 100;
        if (args.size() > 2) {
            extra_ticks = std::stoull(args[2]);
        }
        sim.run_ticks(extra_ticks);

        std::cout << "[FLGODTV] Checkpoint restored from " << path 
                  << ", executed " << extra_ticks << " additional ticks (current tick: "
                  << sim.state().clock().tick() << ", hash: 0x" 
                  << std::hex << sim.compute_state_hash() << std::dec << ")\n";
        return 0;
    }

    if (primary_mode == "--replay") {
        if (args.size() < 2) {
            std::cerr << "Error: --replay requires a replay log filepath\n";
            return 1;
        }
        std::string path = args[1];
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "Error: Could not open replay file " << path << "\n";
            return 1;
        }
        nlohmann::json log_json;
        in >> log_json;
        in.close();

        flgod::ReplayLog log;
        log.from_json(log_json);

        std::string err;
        if (!flgod::ReplayManager::verify(log, &err)) {
            std::cerr << "[FLGODTV] Replay VERIFICATION FAILED: " << err << "\n";
            return 1;
        }
        std::cout << "[FLGODTV] Replay VERIFIED bit-exact for " << log.total_ticks 
                  << " ticks across " << log.checkpoint_ticks.size() << " checkpoints.\n";
        return 0;
    }

    if (primary_mode == "--headless" || primary_mode == "--simulate") {
        uint64_t ticks = 600;
        uint64_t seed = 133701ULL;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--seed" && i + 1 < args.size()) {
                seed = std::stoull(args[i + 1]);
                ++i;
            } else {
                try {
                    ticks = std::stoull(args[i]);
                } catch (...) {}
            }
        }

        std::cout << "[FLGODTV] Running headless simulation for " << ticks 
                  << " ticks with world seed " << seed << "...\n";
        flgod::SimulationConfig cfg;
        cfg.seeds.world_seed = seed;
        flgod::Simulation sim;
        sim.initialize(cfg);
        sim.run_ticks(ticks);
        std::cout << "[FLGODTV] Completed " << ticks << " ticks. Final state hash: 0x"
                  << std::hex << sim.compute_state_hash() << std::dec << "\n";
        return 0;
    }

    std::cerr << "Unknown mode: " << primary_mode << "\n";
    print_usage(argv[0]);
    return 1;
}
