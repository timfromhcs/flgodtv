#include "flgod/ui/telemetry_hud.hpp"
#include <cstdlib>
#include <iostream>

#define ALWAYS_ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " #cond " at " __FILE__ ":" << __LINE__ << std::endl; \
        std::abort(); \
    } \
} while(0)

using namespace flgod;

void test_telemetry_snapshot_capture_and_roundtrip() {
    std::cout << "[Test] Running test_telemetry_snapshot_capture_and_roundtrip..." << std::endl;

    MultiAgentEcosystemConfig config;
    config.fixed_dt = 1.0 / 60.0;
    MultiAgentEcosystem eco;
    eco.initialize(config);

    // Register a colony and spawn 2 agents
    Colony c1(1, "Alpha Colony", Vec3{0.0, 0.0, 0.0}, 25.0);
    eco.agent_manager().register_colony(c1);

    Agent a1(EntityID(1), 1);
    a1.set_position(Vec3{5.0, 1.0, 5.0});
    eco.agent_manager().spawn_agent(a1);

    Agent a2(EntityID(2), 1);
    a2.set_position(Vec3{8.0, 1.0, 8.0});
    eco.agent_manager().spawn_agent(a2);

    EventDetector events;
    SimulationEvent ev;
    ev.tick = 10;
    ev.type = SimulationEventType::LessonTaught;
    ev.priority = 85.0;
    ev.description = "God Fly Shared Nectar Concept";
    events.record_event(ev);

    CameraDirector camera;
    // Step simulation 10 ticks
    for (int i = 0; i < 10; ++i) {
        eco.step();
        camera.step(1.0 / 60.0, i + 1, (i + 1) / 60.0, events, &eco.world_state().world(), eco.agent_manager(), eco.god_fly());
    }

    // Capture telemetry snapshot
    TelemetrySnapshot snap = TelemetryCollector::capture(eco, camera, events, CameraChannel::Cam3_Event);

    ALWAYS_ASSERT(snap.tick == 10);
    ALWAYS_ASSERT(snap.total_agents == 2);
    ALWAYS_ASSERT(snap.alive_agents == 2);
    ALWAYS_ASSERT(snap.colony_count == 1);
    ALWAYS_ASSERT(snap.active_camera == "Cam3_Event");
    ALWAYS_ASSERT(snap.latest_event == "God Fly Shared Nectar Concept");
    ALWAYS_ASSERT(snap.event_priority == 85.0);
    ALWAYS_ASSERT(snap.has_weather == true);
    ALWAYS_ASSERT(snap.has_brain_data == true);
    ALWAYS_ASSERT(snap.brain_connectome_model == "MaleCNS (VNC+Central Brain)");
    ALWAYS_ASSERT(snap.has_language_data == true);

    // Test JSON serialization & roundtrip
    nlohmann::json j = snap.to_json();
    TelemetrySnapshot restored = TelemetrySnapshot::from_json(j);

    ALWAYS_ASSERT(restored.tick == snap.tick);
    ALWAYS_ASSERT(restored.total_agents == snap.total_agents);
    ALWAYS_ASSERT(restored.active_camera == snap.active_camera);
    ALWAYS_ASSERT(restored.latest_event == snap.latest_event);
    ALWAYS_ASSERT(restored.event_priority == snap.event_priority);
    ALWAYS_ASSERT(restored.has_weather == snap.has_weather);
    ALWAYS_ASSERT(restored.brain_connectome_model == snap.brain_connectome_model);

    std::cout << "  -> Telemetry capture and serialization roundtrip passed." << std::endl;
}

void test_telemetry_na_fallback_when_empty() {
    std::cout << "[Test] Running test_telemetry_na_fallback_when_empty..." << std::endl;

    MultiAgentEcosystemConfig config;
    MultiAgentEcosystem eco;
    eco.initialize(config);

    EventDetector empty_events;
    CameraDirector camera;

    TelemetrySnapshot snap = TelemetryCollector::capture(eco, camera, empty_events, CameraChannel::Cam1_GodFly);

    // When no events are present, event description must strictly be "N/A"
    ALWAYS_ASSERT(snap.latest_event == "N/A");
    ALWAYS_ASSERT(snap.event_priority == 0.0);
    ALWAYS_ASSERT(snap.active_camera == "Cam1_GodFly");

    std::cout << "  -> N/A fallback when empty verified (zero fake data)." << std::endl;
}

int main() {
    std::cout << "=== FLGODTV STAGE 19 TELEMETRY HUD TEST SUITE ===" << std::endl;
    test_telemetry_snapshot_capture_and_roundtrip();
    test_telemetry_na_fallback_when_empty();
    std::cout << "=== ALL STAGE 19 TELEMETRY HUD TESTS PASSED ===" << std::endl;
    return 0;
}
