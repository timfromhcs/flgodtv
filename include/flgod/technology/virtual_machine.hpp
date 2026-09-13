#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod::technology {

enum class VMOpcode : uint8_t {
    HALT = 0,
    NOP = 1,
    LOAD = 2,     // LOAD Rd, [addr]
    STORE = 3,    // STORE [addr], Rs
    MOV = 4,      // MOV Rd, Rs
    MOVI = 5,     // MOVI Rd, imm
    ADD = 6,      // ADD Rd, Rs
    SUB = 7,      // SUB Rd, Rs
    MUL = 8,      // MUL Rd, Rs
    DIV = 9,      // DIV Rd, Rs (safe divide-by-zero protection)
    JMP = 10,     // JMP target_pc
    JZ = 11,      // JZ R_test, target_pc
    JNZ = 12,     // JNZ R_test, target_pc
    READ_DEV = 13,// READ_DEV port, Rd
    WRITE_DEV = 14,// WRITE_DEV port, Rs
    SEND_NET = 15 // SEND_NET dst, Rs
};

struct VMInstruction {
    VMOpcode opcode{VMOpcode::NOP};
    uint8_t reg_dest{0};    // 0-7
    uint8_t reg_src{0};     // 0-7
    int32_t immediate{0};   // Immediate constant or address
};

enum class VMStatus : uint8_t {
    Ready = 0,
    Running = 1,
    Halted = 2,
    CycleLimitExceeded = 3,
    MemoryFault = 4,
    InvalidInstruction = 5
};

inline const char* vm_status_to_string(VMStatus st) {
    switch (st) {
        case VMStatus::Ready: return "Ready";
        case VMStatus::Running: return "Running";
        case VMStatus::Halted: return "Halted";
        case VMStatus::CycleLimitExceeded: return "CycleLimitExceeded";
        case VMStatus::MemoryFault: return "MemoryFault";
        case VMStatus::InvalidInstruction: return "InvalidInstruction";
    }
    return "Unknown";
}

// SECTION 59: "Safe virtual execution layer... but never receive arbitrary host-level execution privileges."
class VirtualMachine {
public:
    static constexpr size_t NUM_REGISTERS = 8;
    static constexpr size_t MEMORY_SIZE = 256; // 256 32-bit words (1 KB)
    static constexpr uint32_t MAX_CYCLES_PER_STEP = 1000;

private:
    std::array<int32_t, NUM_REGISTERS> m_registers{};
    std::array<int32_t, MEMORY_SIZE> m_memory{};
    std::vector<VMInstruction> m_program;
    uint32_t m_pc{0};
    VMStatus m_status{VMStatus::Ready};
    uint64_t m_total_cycles_executed{0};

    // Virtual I/O ports
    std::unordered_map<uint32_t, int32_t> m_io_ports;

public:
    VirtualMachine() {
        reset();
    }

    void reset() noexcept {
        m_registers.fill(0);
        m_memory.fill(0);
        m_pc = 0;
        m_status = VMStatus::Ready;
        m_total_cycles_executed = 0;
    }

    void load_program(const std::vector<VMInstruction>& program) {
        m_program = program;
        reset();
    }

    [[nodiscard]] VMStatus status() const noexcept { return m_status; }
    [[nodiscard]] uint32_t pc() const noexcept { return m_pc; }
    [[nodiscard]] uint64_t total_cycles() const noexcept { return m_total_cycles_executed; }

    [[nodiscard]] int32_t get_register(uint8_t reg) const {
        if (reg >= NUM_REGISTERS) return 0;
        return m_registers[reg];
    }

    void set_register(uint8_t reg, int32_t val) {
        if (reg < NUM_REGISTERS) m_registers[reg] = val;
    }

    [[nodiscard]] int32_t get_memory(uint32_t addr) const {
        if (addr >= MEMORY_SIZE) return 0;
        return m_memory[addr];
    }

    void set_memory(uint32_t addr, int32_t val) {
        if (addr < MEMORY_SIZE) m_memory[addr] = val;
    }

    void set_io_port(uint32_t port, int32_t val) {
        m_io_ports[port] = val;
    }

    [[nodiscard]] int32_t get_io_port(uint32_t port) const {
        auto it = m_io_ports.find(port);
        if (it != m_io_ports.end()) return it->second;
        return 0;
    }

    // Execute bounded cycles with anti-hang sandboxing
    VMStatus execute_steps(uint32_t max_cycles = MAX_CYCLES_PER_STEP) {
        if (m_status == VMStatus::Halted || m_program.empty()) {
            return m_status;
        }

        m_status = VMStatus::Running;
        uint32_t cycles = 0;

        while (cycles < max_cycles && m_status == VMStatus::Running) {
            if (m_pc >= m_program.size()) {
                m_status = VMStatus::Halted;
                break;
            }

            const auto& inst = m_program[m_pc];
            cycles++;
            m_total_cycles_executed++;

            switch (inst.opcode) {
                case VMOpcode::HALT:
                    m_status = VMStatus::Halted;
                    break;

                case VMOpcode::NOP:
                    m_pc++;
                    break;

                case VMOpcode::LOAD: {
                    uint32_t addr = static_cast<uint32_t>(inst.immediate);
                    if (addr >= MEMORY_SIZE) {
                        m_status = VMStatus::MemoryFault;
                        break;
                    }
                    if (inst.reg_dest < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] = m_memory[addr];
                    }
                    m_pc++;
                    break;
                }

                case VMOpcode::STORE: {
                    uint32_t addr = static_cast<uint32_t>(inst.immediate);
                    if (addr >= MEMORY_SIZE) {
                        m_status = VMStatus::MemoryFault;
                        break;
                    }
                    if (inst.reg_src < NUM_REGISTERS) {
                        m_memory[addr] = m_registers[inst.reg_src];
                    }
                    m_pc++;
                    break;
                }

                case VMOpcode::MOV:
                    if (inst.reg_dest < NUM_REGISTERS && inst.reg_src < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] = m_registers[inst.reg_src];
                    }
                    m_pc++;
                    break;

                case VMOpcode::MOVI:
                    if (inst.reg_dest < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] = inst.immediate;
                    }
                    m_pc++;
                    break;

                case VMOpcode::ADD:
                    if (inst.reg_dest < NUM_REGISTERS && inst.reg_src < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] += m_registers[inst.reg_src];
                    }
                    m_pc++;
                    break;

                case VMOpcode::SUB:
                    if (inst.reg_dest < NUM_REGISTERS && inst.reg_src < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] -= m_registers[inst.reg_src];
                    }
                    m_pc++;
                    break;

                case VMOpcode::MUL:
                    if (inst.reg_dest < NUM_REGISTERS && inst.reg_src < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] *= m_registers[inst.reg_src];
                    }
                    m_pc++;
                    break;

                case VMOpcode::DIV:
                    if (inst.reg_dest < NUM_REGISTERS && inst.reg_src < NUM_REGISTERS) {
                        int32_t denom = m_registers[inst.reg_src];
                        if (denom != 0) {
                            m_registers[inst.reg_dest] /= denom;
                        } else {
                            m_registers[inst.reg_dest] = 0; // Safe divide-by-zero protection
                        }
                    }
                    m_pc++;
                    break;

                case VMOpcode::JMP:
                    if (inst.immediate >= 0 && static_cast<size_t>(inst.immediate) < m_program.size()) {
                        m_pc = static_cast<uint32_t>(inst.immediate);
                    } else {
                        m_status = VMStatus::InvalidInstruction;
                    }
                    break;

                case VMOpcode::JZ:
                    if (inst.reg_src < NUM_REGISTERS && m_registers[inst.reg_src] == 0) {
                        if (inst.immediate >= 0 && static_cast<size_t>(inst.immediate) < m_program.size()) {
                            m_pc = static_cast<uint32_t>(inst.immediate);
                        } else {
                            m_status = VMStatus::InvalidInstruction;
                        }
                    } else {
                        m_pc++;
                    }
                    break;

                case VMOpcode::JNZ:
                    if (inst.reg_src < NUM_REGISTERS && m_registers[inst.reg_src] != 0) {
                        if (inst.immediate >= 0 && static_cast<size_t>(inst.immediate) < m_program.size()) {
                            m_pc = static_cast<uint32_t>(inst.immediate);
                        } else {
                            m_status = VMStatus::InvalidInstruction;
                        }
                    } else {
                        m_pc++;
                    }
                    break;

                case VMOpcode::READ_DEV:
                    if (inst.reg_dest < NUM_REGISTERS) {
                        m_registers[inst.reg_dest] = get_io_port(static_cast<uint32_t>(inst.immediate));
                    }
                    m_pc++;
                    break;

                case VMOpcode::WRITE_DEV:
                    if (inst.reg_src < NUM_REGISTERS) {
                        set_io_port(static_cast<uint32_t>(inst.immediate), m_registers[inst.reg_src]);
                    }
                    m_pc++;
                    break;

                default:
                    m_status = VMStatus::InvalidInstruction;
                    break;
            }
        }

        if (cycles >= max_cycles && m_status == VMStatus::Running) {
            m_status = VMStatus::CycleLimitExceeded;
        }

        return m_status;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"pc", m_pc},
            {"status", vm_status_to_string(m_status)},
            {"total_cycles", m_total_cycles_executed},
            {"registers", m_registers},
            {"program_size", m_program.size()}
        };
    }
};

} // namespace flgod::technology
