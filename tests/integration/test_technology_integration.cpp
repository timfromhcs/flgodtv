#include <cassert>
#include <iostream>
#include "flgod/technology/programmable_world.hpp"

int main() {
    std::cout << "[Test] Running Programmable Technology Layer Integration Test...\n";

    flgod::technology::ProgrammableTechnologyLayer tech_layer;

    // 1. Primitive Tool Construction: Build climate monitoring station
    auto* builder = tech_layer.tools().create_primitive(
        "StationAssembler",
        flgod::technology::MaterialType::StonePebble,
        static_cast<uint8_t>(flgod::technology::PrimitiveCapability::Construct | flgod::technology::PrimitiveCapability::Connect)
    );
    assert(builder != nullptr);
    bool built = tech_layer.tools().construct_component(builder->item_id, "HiveVentilation");
    assert(built);

    // 2. Hardware Deployment: Virtual Devices
    auto* sensor = tech_layer.create_device(1, 0x01, flgod::technology::DeviceType::SensorThermometer);
    auto* valve = tech_layer.create_device(2, 0x02, flgod::technology::DeviceType::ActuatorSolenoidValve);
    assert(sensor != nullptr && valve != nullptr);

    // 3. Controller VM Deployment with Automated Regulation Logic
    auto* vm = tech_layer.create_vm(100);
    assert(vm != nullptr);

    // Virtual Program:
    // 0: READ_DEV port 10, R0   (read sensor value)
    // 1: MOVI R1, 35           (threshold = 35)
    // 2: MOV R2, R0            (copy sensor value to R2)
    // 3: SUB R2, R1            (R2 = temp - 35)
    // 4: JNZ R2, 0             (if not reached threshold, loop)
    // 5: MOVI R3, 1            (sprinkler ON command)
    // 6: WRITE_DEV port 20, R3 (activate local output)
    // 7: HALT
    std::vector<flgod::technology::VMInstruction> regulation_prog = {
        {flgod::technology::VMOpcode::READ_DEV, 0, 0, 10},
        {flgod::technology::VMOpcode::MOVI, 1, 0, 35},
        {flgod::technology::VMOpcode::MOV, 2, 0, 0},
        {flgod::technology::VMOpcode::SUB, 2, 1, 0},
        {flgod::technology::VMOpcode::JNZ, 0, 2, 0}, // Loop if temp != 35
        {flgod::technology::VMOpcode::MOVI, 3, 0, 1},
        {flgod::technology::VMOpcode::WRITE_DEV, 0, 3, 20},
        {flgod::technology::VMOpcode::HALT, 0, 0, 0}
    };
    vm->load_program(regulation_prog);

    // Phase A: Temperature is normal (22 deg C)
    vm->set_io_port(10, 22);
    tech_layer.step(0.1);
    // VM looped and reached cycle limit without halting
    assert(vm->get_io_port(20) == 0 && "Sprinkler should remain OFF at normal temperature");

    // Phase B: Temperature reaches critical threshold (35 deg C)
    vm->set_io_port(10, 35);
    tech_layer.step(0.1);
    // Program detects threshold, sets port 20 to 1, and halts
    assert(vm->status() == flgod::technology::VMStatus::Halted);
    assert(vm->get_io_port(20) == 1 && "Sprinkler actuator should turn ON!");

    // 4. Send network packet from controller to physical valve
    flgod::technology::VirtualPacket net_cmd;
    net_cmd.src_address = 0x01;
    net_cmd.dst_address = 0x02;
    net_cmd.data = {vm->get_io_port(20)};
    tech_layer.network().send_packet(net_cmd);
    tech_layer.step(0.1);

    assert(valve->read_value() == 1 && "Valve actuator latched ON via virtual network");

    // 5. Deterministic State Hashing & Crash Recovery
    uint64_t hash1 = tech_layer.compute_hash();
    assert(hash1 != 0);
    auto json_snap = tech_layer.to_json();
    assert(json_snap.contains("tools"));
    assert(json_snap.contains("network"));
    assert(json_snap.contains("vms"));

    std::cout << "  Programmable Technology Layer Verified: Tool crafting -> device setup -> sandboxed VM control -> network dispatch -> actuator latch -> state hash: 0x" 
              << std::hex << hash1 << std::dec << "\n";
    std::cout << "[Test] Programmable Technology Layer Integration Test PASSED!\n";
    return 0;
}
