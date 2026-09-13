#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod::technology {

struct VirtualPacket {
    uint16_t src_address{0};
    uint16_t dst_address{0};
    uint8_t port{0};
    std::vector<int32_t> data;
    double timestamp{0.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"src", src_address},
            {"dst", dst_address},
            {"port", port},
            {"data", data},
            {"time", timestamp}
        };
    }
};

enum class DeviceType : uint8_t {
    SensorThermometer = 0,
    SensorPhotodetector = 1,
    ActuatorMotor = 2,
    ActuatorSolenoidValve = 3,
    VirtualNodeVM = 4
};

inline const char* device_type_to_string(DeviceType dt) {
    switch (dt) {
        case DeviceType::SensorThermometer: return "SensorThermometer";
        case DeviceType::SensorPhotodetector: return "SensorPhotodetector";
        case DeviceType::ActuatorMotor: return "ActuatorMotor";
        case DeviceType::ActuatorSolenoidValve: return "ActuatorSolenoidValve";
        case DeviceType::VirtualNodeVM: return "VirtualNodeVM";
    }
    return "Unknown";
}

class VirtualDevice {
private:
    uint32_t m_device_id{0};
    uint16_t m_address{0};
    DeviceType m_type{DeviceType::SensorThermometer};
    std::deque<VirtualPacket> m_inbox;
    int32_t m_current_value{0};
    bool m_is_powered{true};

public:
    VirtualDevice(uint32_t id, uint16_t addr, DeviceType type)
        : m_device_id(id), m_address(addr), m_type(type) {}

    [[nodiscard]] uint32_t id() const noexcept { return m_device_id; }
    [[nodiscard]] uint16_t address() const noexcept { return m_address; }
    [[nodiscard]] DeviceType type() const noexcept { return m_type; }
    [[nodiscard]] bool is_powered() const noexcept { return m_is_powered; }
    void set_power(bool on) noexcept { m_is_powered = on; }

    [[nodiscard]] int32_t read_value() const noexcept { return m_current_value; }
    void write_value(int32_t val) noexcept { m_current_value = val; }

    void receive_packet(const VirtualPacket& pkt) {
        if (!m_is_powered) return;
        m_inbox.push_back(pkt);
        if (m_inbox.size() > 64) {
            m_inbox.pop_front(); // FIFO drop
        }
        if (!pkt.data.empty()) {
            m_current_value = pkt.data[0]; // Actuator latch
        }
    }

    [[nodiscard]] bool has_packet() const noexcept { return !m_inbox.empty(); }
    VirtualPacket pop_packet() {
        if (m_inbox.empty()) return {};
        VirtualPacket pkt = m_inbox.front();
        m_inbox.pop_front();
        return pkt;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"device_id", m_device_id},
            {"address", m_address},
            {"type", device_type_to_string(m_type)},
            {"value", m_current_value},
            {"powered", m_is_powered},
            {"inbox_size", m_inbox.size()}
        };
    }
};

class VirtualNetwork {
private:
    std::unordered_map<uint16_t, VirtualDevice*> m_devices; // address -> device
    std::deque<VirtualPacket> m_packet_queue;
    uint64_t m_packets_routed{0};
    uint64_t m_packets_dropped{0};

public:
    VirtualNetwork() = default;

    bool attach_device(VirtualDevice* device) {
        if (!device) return false;
        m_devices[device->address()] = device;
        return true;
    }

    void detach_device(uint16_t address) {
        m_devices.erase(address);
    }

    bool send_packet(const VirtualPacket& pkt) {
        if (m_packet_queue.size() >= 1024) {
            m_packets_dropped++;
            return false;
        }
        m_packet_queue.push_back(pkt);
        return true;
    }

    // Step the network routing cycle
    void step_network() {
        size_t count = m_packet_queue.size();
        for (size_t i = 0; i < count; ++i) {
            VirtualPacket pkt = m_packet_queue.front();
            m_packet_queue.pop_front();

            // Broadcast or Unicast
            if (pkt.dst_address == 0xFFFF) {
                // Broadcast to all attached devices
                for (auto& [_, dev] : m_devices) {
                    if (dev->address() != pkt.src_address) {
                        dev->receive_packet(pkt);
                    }
                }
                m_packets_routed++;
            } else {
                auto it = m_devices.find(pkt.dst_address);
                if (it != m_devices.end()) {
                    it->second->receive_packet(pkt);
                    m_packets_routed++;
                } else {
                    m_packets_dropped++;
                }
            }
        }
    }

    [[nodiscard]] size_t attached_device_count() const noexcept { return m_devices.size(); }
    [[nodiscard]] uint64_t packets_routed() const noexcept { return m_packets_routed; }
    [[nodiscard]] uint64_t packets_dropped() const noexcept { return m_packets_dropped; }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"attached_devices", m_devices.size()},
            {"packets_routed", m_packets_routed},
            {"packets_dropped", m_packets_dropped},
            {"queue_size", m_packet_queue.size()}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("packets_routed")) m_packets_routed = j["packets_routed"].get<uint64_t>();
        if (j.contains("packets_dropped")) m_packets_dropped = j["packets_dropped"].get<uint64_t>();
    }
};

} // namespace flgod::technology
