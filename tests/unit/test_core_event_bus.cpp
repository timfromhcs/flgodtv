#include "flgod/core/event_bus.hpp"
#include <iostream>

struct CustomPayloadEvent : public flgod::Event<CustomPayloadEvent> {
    int value{0};
};

int main() {
    std::cout << "[TEST] Running test_core_event_bus..." << std::endl;

    flgod::EventBus bus;

    int tick_starts_received = 0;
    int tick_ends_received = 0;
    int custom_payload_sum = 0;

    bus.subscribe<flgod::TickStartEvent>([&]([[maybe_unused]] const flgod::TickStartEvent& ev) {
        tick_starts_received++;
    });

    bus.subscribe<flgod::TickEndEvent>([&]([[maybe_unused]] const flgod::TickEndEvent& ev) {
        tick_ends_received++;
    });

    bus.subscribe<CustomPayloadEvent>([&](const CustomPayloadEvent& ev) {
        custom_payload_sum += ev.value;
    });

    // 1. Immediate publish
    flgod::TickStartEvent start_ev;
    bus.publish_immediate(start_ev);
    if (tick_starts_received != 1) {
        std::cerr << "FAILED: Immediate publish was not received" << std::endl;
        return 1;
    }

    // 2. Queued events
    CustomPayloadEvent c1; c1.value = 10;
    CustomPayloadEvent c2; c2.value = 25;
    bus.enqueue(c1);
    bus.enqueue(c2);

    if (bus.queued_count() != 2) {
        std::cerr << "FAILED: Expected queued_count 2, got " << bus.queued_count() << std::endl;
        return 1;
    }
    if (custom_payload_sum != 0) {
        std::cerr << "FAILED: Queued event should not be executed before flush" << std::endl;
        return 1;
    }

    size_t flushed = bus.flush(1);
    if (flushed != 2) {
        std::cerr << "FAILED: Expected 2 events flushed, got " << flushed << std::endl;
        return 1;
    }
    if (custom_payload_sum != 35) {
        std::cerr << "FAILED: Expected custom_payload_sum 35, got " << custom_payload_sum << std::endl;
        return 1;
    }
    if (bus.queued_count() != 0) {
        std::cerr << "FAILED: Queue should be empty after flush" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_core_event_bus passed successfully." << std::endl;
    return 0;
}
