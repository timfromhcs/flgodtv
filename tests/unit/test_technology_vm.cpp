#include <cassert>
#include <iostream>
#include "flgod/technology/virtual_machine.hpp"

int main() {
    std::cout << "[Test] Running Virtual Machine Sandboxed Execution Unit Tests...\n";

    flgod::technology::VirtualMachine vm;

    // 1. Basic Arithmetic Program:
    // R0 = 10, R1 = 25, R2 = R0 + R1 (35), R3 = R2 - 5 (30), R4 = R3 * 2 (60), R5 = R4 / 4 (15)
    std::vector<flgod::technology::VMInstruction> prog_math = {
        {flgod::technology::VMOpcode::MOVI, 0, 0, 10},
        {flgod::technology::VMOpcode::MOVI, 1, 0, 25},
        {flgod::technology::VMOpcode::ADD, 2, 0, 0},  // R2 += R0 (10)
        {flgod::technology::VMOpcode::ADD, 2, 1, 0},  // R2 += R1 (35)
        {flgod::technology::VMOpcode::MOVI, 3, 0, 5},
        {flgod::technology::VMOpcode::MOV, 4, 2, 0},   // R4 = R2
        {flgod::technology::VMOpcode::SUB, 4, 3, 0},   // R4 = 35 - 5 = 30
        {flgod::technology::VMOpcode::MOVI, 5, 0, 2},
        {flgod::technology::VMOpcode::MUL, 4, 5, 0},   // R4 = 30 * 2 = 60
        {flgod::technology::VMOpcode::MOVI, 6, 0, 4},
        {flgod::technology::VMOpcode::DIV, 4, 6, 0},   // R4 = 60 / 4 = 15
        {flgod::technology::VMOpcode::HALT, 0, 0, 0}
    };

    vm.load_program(prog_math);
    auto status1 = vm.execute_steps(100);
    assert(status1 == flgod::technology::VMStatus::Halted);
    (void)status1;
    assert(vm.get_register(4) == 15);
    std::cout << "  Arithmetic program verified: R4 = " << vm.get_register(4) << "\n";

    // 2. Safe Divide-by-Zero Protection
    std::vector<flgod::technology::VMInstruction> prog_div_zero = {
        {flgod::technology::VMOpcode::MOVI, 0, 0, 100},
        {flgod::technology::VMOpcode::MOVI, 1, 0, 0},
        {flgod::technology::VMOpcode::DIV, 0, 1, 0},   // Divide by zero!
        {flgod::technology::VMOpcode::HALT, 0, 0, 0}
    };
    vm.load_program(prog_div_zero);
    auto status_div = vm.execute_steps(10);
    assert(status_div == flgod::technology::VMStatus::Halted);
    (void)status_div;
    assert(vm.get_register(0) == 0 && "Divide-by-zero must safely return 0 without crashing host!");

    // 3. Sandboxed Memory Access & Bounds Checking
    std::vector<flgod::technology::VMInstruction> prog_mem = {
        {flgod::technology::VMOpcode::MOVI, 0, 0, 777},
        {flgod::technology::VMOpcode::STORE, 0, 0, 50}, // STORE [50], R0
        {flgod::technology::VMOpcode::LOAD, 1, 0, 50},  // LOAD R1, [50]
        {flgod::technology::VMOpcode::HALT, 0, 0, 0}
    };
    vm.load_program(prog_mem);
    vm.execute_steps(10);
    assert(vm.get_memory(50) == 777);
    assert(vm.get_register(1) == 777);

    // Out-of-bounds memory access (e.g. addr = 300 > 256)
    std::vector<flgod::technology::VMInstruction> prog_oob = {
        {flgod::technology::VMOpcode::LOAD, 0, 0, 300} // Out of bounds!
    };
    vm.load_program(prog_oob);
    auto status_oob = vm.execute_steps(10);
    assert(status_oob == flgod::technology::VMStatus::MemoryFault);
    (void)status_oob;
    std::cout << "  Memory boundary fault verified: rejected out-of-bounds address\n";

    // 4. Anti-Hang Cycle Limit Enforcement
    // Infinite loop: JMP 0
    std::vector<flgod::technology::VMInstruction> prog_infinite = {
        {flgod::technology::VMOpcode::JMP, 0, 0, 0}
    };
    vm.load_program(prog_infinite);
    auto status_inf = vm.execute_steps(1000);
    assert(status_inf == flgod::technology::VMStatus::CycleLimitExceeded);
    (void)status_inf;
    assert(vm.total_cycles() == 1000);
    std::cout << "  Infinite loop safely bounded: halted at exactly 1000 cycles\n";

    std::cout << "[Test] Virtual Machine Sandboxed Execution Unit Tests PASSED!\n";
    return 0;
}
