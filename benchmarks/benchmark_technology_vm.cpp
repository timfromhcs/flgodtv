#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "flgod/technology/programmable_world.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << "  FLGODTV PROGRAMMABLE TECHNOLOGY BENCHMARK\n";
    std::cout << "========================================================\n";

    flgod::technology::ProgrammableTechnologyLayer tech_layer;

    // 1. VM Execution Speed Benchmark
    auto* vm = tech_layer.create_vm(1);
    std::vector<flgod::technology::VMInstruction> loop_prog = {
        {flgod::technology::VMOpcode::MOVI, 0, 0, 0},
        {flgod::technology::VMOpcode::MOVI, 1, 0, 1},
        {flgod::technology::VMOpcode::ADD, 0, 1, 0},
        {flgod::technology::VMOpcode::JMP, 0, 0, 2} // Loop at instruction 2
    };
    vm->load_program(loop_prog);

    const int VM_STEPS = 5000;
    auto t_vm_start = std::chrono::steady_clock::now();

    for (int i = 0; i < VM_STEPS; ++i) {
        vm->execute_steps(1000); // 1000 cycles per step
    }

    auto t_vm_end = std::chrono::steady_clock::now();
    double vm_time_ms = std::chrono::duration<double, std::milli>(t_vm_end - t_vm_start).count();
    double total_instructions = static_cast<double>(VM_STEPS) * 1000.0;
    double mips = (total_instructions / 1e6) / (vm_time_ms / 1000.0);

    std::cout << "VM Instructions: " << total_instructions << " in " << vm_time_ms << " ms\n";
    std::cout << "VM Performance:  " << mips << " MIPS\n";

    // 2. Virtual Network Routing Benchmark
    auto* dev1 = tech_layer.create_device(1, 0x01, flgod::technology::DeviceType::SensorThermometer);
    auto* dev2 = tech_layer.create_device(2, 0x02, flgod::technology::DeviceType::ActuatorMotor);
    (void)dev1; (void)dev2;

    const int PACKET_COUNT = 100000;
    auto t_net_start = std::chrono::steady_clock::now();

    for (int i = 0; i < PACKET_COUNT; ++i) {
        flgod::technology::VirtualPacket pkt;
        pkt.src_address = 0x01;
        pkt.dst_address = 0x02;
        pkt.data = {i};
        tech_layer.network().send_packet(pkt);
        if ((i % 500) == 0) {
            tech_layer.network().step_network();
        }
    }
    tech_layer.network().step_network();

    auto t_net_end = std::chrono::steady_clock::now();
    double net_time_ms = std::chrono::duration<double, std::milli>(t_net_end - t_net_start).count();
    double packets_per_sec = (static_cast<double>(PACKET_COUNT) / (net_time_ms / 1000.0));

    std::cout << "Network Routing: " << PACKET_COUNT << " packets in " << net_time_ms << " ms\n";
    std::cout << "Network Speed:   " << packets_per_sec << " packets/sec\n";

    // 3. Save JSON Evidence
    nlohmann::json bench_json;
    bench_json["vm_cycles_executed"] = total_instructions;
    bench_json["vm_time_ms"] = vm_time_ms;
    bench_json["vm_mips"] = mips;
    bench_json["packets_routed"] = PACKET_COUNT;
    bench_json["network_time_ms"] = net_time_ms;
    bench_json["packets_per_second"] = packets_per_sec;

    std::filesystem::create_directories("evidence/windows");
    std::string out_path = "evidence/windows/technology_benchmark.json";
    std::ofstream out(out_path);
    out << bench_json.dump(2) << std::endl;
    out.close();

    std::cout << "Saved benchmark evidence to: " << out_path << "\n";
    std::cout << "========================================================\n";
    return 0;
}
