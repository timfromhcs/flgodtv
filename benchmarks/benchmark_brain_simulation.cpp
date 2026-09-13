#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "flgod/brain/malecns_adapter.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << "  FLGODTV CONNECTOME BRAIN SIMULATION BENCHMARK\n";
    std::cout << "========================================================\n";

    std::string csv_path = "malecns/data-raw/2023-27-2 soma_sides.csv";
    const size_t BENCHMARK_SOMAS = 10000;

    auto t_load_start = std::chrono::steady_clock::now();
    flgod::brain::MaleCNSAdapter brain(csv_path, BENCHMARK_SOMAS);
    auto t_load_end = std::chrono::steady_clock::now();

    double load_time_ms = std::chrono::duration<double, std::milli>(t_load_end - t_load_start).count();
    std::cout << "Connectome Load Time: " << load_time_ms << " ms (" << brain.soma_count() << " somas)\n";

    // Neural Simulation Loop Benchmark
    const int STEPS = 1000;
    flgod::brain::BrainSensoryInput input;
    input.odor_sugar_intensity = 0.7;
    input.left_eye_sectors.fill(0.8);
    input.right_eye_sectors.fill(0.5);
    flgod::brain::BrainMotorOutput motor_out;

    auto t_sim_start = std::chrono::steady_clock::now();

    for (int i = 0; i < STEPS; ++i) {
        brain.step(0.01, input, motor_out);
    }

    auto t_sim_end = std::chrono::steady_clock::now();
    double sim_time_ms = std::chrono::duration<double, std::milli>(t_sim_end - t_sim_start).count();
    double steps_per_sec = (static_cast<double>(STEPS) / (sim_time_ms / 1000.0));
    double soma_updates_per_sec = steps_per_sec * brain.soma_count();

    std::cout << "Simulated Steps:      " << STEPS << " in " << sim_time_ms << " ms\n";
    std::cout << "Simulation Rate:      " << steps_per_sec << " steps/sec (~" << (steps_per_sec / 100.0) << "x realtime at 100Hz)\n";
    std::cout << "Soma Update Rate:     " << soma_updates_per_sec << " soma-updates/sec\n";
    std::cout << "Active Neurons:       " << brain.active_neuron_count() << " / " << brain.soma_count() << "\n";

    // Save JSON Evidence
    nlohmann::json bench_json;
    bench_json["soma_count"] = brain.soma_count();
    bench_json["load_time_ms"] = load_time_ms;
    bench_json["steps_simulated"] = STEPS;
    bench_json["simulation_time_ms"] = sim_time_ms;
    bench_json["steps_per_second"] = steps_per_sec;
    bench_json["soma_updates_per_second"] = soma_updates_per_sec;
    bench_json["active_neurons"] = brain.active_neuron_count();

    std::filesystem::create_directories("evidence/windows");
    std::string out_path = "evidence/windows/brain_benchmark.json";
    std::ofstream out(out_path);
    out << bench_json.dump(2) << std::endl;
    out.close();

    std::cout << "Saved benchmark evidence to: " << out_path << "\n";
    std::cout << "========================================================\n";
    return 0;
}
