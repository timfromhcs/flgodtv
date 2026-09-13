#include <cassert>
#include <iostream>
#include "flgod/technology/virtual_network.hpp"

int main() {
    std::cout << "[Test] Running Virtual Devices & Network Unit Tests...\n";

    flgod::technology::VirtualNetwork net;

    // 1. Create and attach virtual devices
    flgod::technology::VirtualDevice sensor(1, 0x1001, flgod::technology::DeviceType::SensorThermometer);
    flgod::technology::VirtualDevice valve(2, 0x1002, flgod::technology::DeviceType::ActuatorSolenoidValve);
    flgod::technology::VirtualDevice monitor(3, 0x1003, flgod::technology::DeviceType::VirtualNodeVM);

    sensor.write_value(38); // 38 deg C
    assert(sensor.read_value() == 38);

    assert(net.attach_device(&sensor));
    assert(net.attach_device(&valve));
    assert(net.attach_device(&monitor));
    assert(net.attached_device_count() == 3);

    // 2. Unicast packet routing: Sensor -> Valve
    flgod::technology::VirtualPacket pkt1;
    pkt1.src_address = 0x1001;
    pkt1.dst_address = 0x1002;
    pkt1.data = {1}; // Command: open valve
    pkt1.timestamp = 1.0;

    assert(net.send_packet(pkt1));
    assert(!valve.has_packet()); // Before step

    net.step_network();
    assert(valve.has_packet()); // Delivered after step
    assert(net.packets_routed() == 1);

    auto rec_pkt = valve.pop_packet();
    assert(rec_pkt.src_address == 0x1001);
    assert(valve.read_value() == 1 && "Actuator should latch packet payload value");

    // 3. Broadcast packet routing: Sensor -> All devices (0xFFFF)
    flgod::technology::VirtualPacket bcast;
    bcast.src_address = 0x1001;
    bcast.dst_address = 0xFFFF;
    bcast.data = {42};
    bcast.timestamp = 2.0;

    net.send_packet(bcast);
    net.step_network();

    assert(valve.has_packet());
    assert(monitor.has_packet());
    assert(!sensor.has_packet() && "Sender should not receive its own broadcast");

    // 4. Packet drop on invalid destination address
    flgod::technology::VirtualPacket bad_pkt;
    bad_pkt.src_address = 0x1001;
    bad_pkt.dst_address = 0x9999; // Non-existent
    bad_pkt.data = {0};
    net.send_packet(bad_pkt);
    net.step_network();
    assert(net.packets_dropped() == 1);

    // 5. Power state control
    valve.set_power(false); // Power off
    assert(!valve.is_powered());
    flgod::technology::VirtualPacket pkt_off;
    pkt_off.src_address = 0x1001;
    pkt_off.dst_address = 0x1002;
    pkt_off.data = {99};
    net.send_packet(pkt_off);
    net.step_network();
    assert(!valve.has_packet() && "Unpowered device should ignore incoming packets");

    std::cout << "[Test] Virtual Devices & Network Unit Tests PASSED!\n";
    return 0;
}
