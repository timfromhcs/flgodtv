#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include "flgod/technology/tool_system.hpp"
#include "flgod/technology/virtual_machine.hpp"
#include "flgod/technology/virtual_network.hpp"

namespace flgod::technology {

class ProgrammableTechnologyLayer {
private:
    ToolSystem m_tools;
    VirtualNetwork m_network;
    std::unordered_map<uint32_t, VirtualMachine> m_vms;
    std::unordered_map<uint32_t, std::unique_ptr<VirtualDevice>> m_devices;

public:
    ProgrammableTechnologyLayer() = default;

    [[nodiscard]] ToolSystem& tools() noexcept { return m_tools; }
    [[nodiscard]] const ToolSystem& tools() const noexcept { return m_tools; }

    [[nodiscard]] VirtualNetwork& network() noexcept { return m_network; }
    [[nodiscard]] const VirtualNetwork& network() const noexcept { return m_network; }

    VirtualMachine* create_vm(uint32_t vm_id) {
        auto res = m_vms.emplace(vm_id, VirtualMachine());
        return &(res.first->second);
    }

    [[nodiscard]] VirtualMachine* get_vm(uint32_t vm_id) {
        auto it = m_vms.find(vm_id);
        if (it != m_vms.end()) return &it->second;
        return nullptr;
    }

    VirtualDevice* create_device(uint32_t id, uint16_t addr, DeviceType type) {
        auto dev = std::make_unique<VirtualDevice>(id, addr, type);
        VirtualDevice* ptr = dev.get();
        m_network.attach_device(ptr);
        m_devices[id] = std::move(dev);
        return ptr;
    }

    void step(double dt) {
        (void)dt;
        // 1. Step all active virtual machines within strict cycle bounds
        for (auto& [_, vm] : m_vms) {
            if (vm.status() == VMStatus::Running || vm.status() == VMStatus::Ready) {
                vm.execute_steps(VirtualMachine::MAX_CYCLES_PER_STEP);
            }
        }

        // 2. Step virtual network routing
        m_network.step_network();
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix(m_tools.item_count());
        mix(m_network.attached_device_count());
        mix(m_network.packets_routed());
        mix(m_vms.size());
        for (const auto& [id, vm] : m_vms) {
            mix(id);
            mix(vm.pc());
            mix(vm.total_cycles());
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json vms_json = nlohmann::json::object();
        for (const auto& [id, vm] : m_vms) {
            vms_json[std::to_string(id)] = vm.to_json();
        }
        return {
            {"tools", m_tools.to_json()},
            {"network", m_network.to_json()},
            {"vms", vms_json}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("network")) {
            m_network.from_json(j["network"]);
        }
        if (j.contains("vms") && j["vms"].is_object()) {
            for (const auto& [id_str, vm_j] : j["vms"].items()) {
                uint32_t id = static_cast<uint32_t>(std::stoul(id_str));
                auto it = m_vms.find(id);
                if (it != m_vms.end()) {
                    it->second.from_json(vm_j);
                }
            }
        }
    }
};

} // namespace flgod::technology
