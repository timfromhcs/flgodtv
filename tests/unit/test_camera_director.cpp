#include "flgod/camera/event_detector.hpp"
#include "flgod/camera/camera_types.hpp"
#include "flgod/camera/camera_director.hpp"
#include <cstdlib>
#include <cmath>
#include <iostream>

#define ALWAYS_ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " #cond " at " __FILE__ ":" << __LINE__ << std::endl; \
        std::abort(); \
    } \
} while(0)

using namespace flgod;

void test_event_detector_priority_and_tie_breaking() {
    std::cout << "[Test] Running test_event_detector_priority_and_tie_breaking..." << std::endl;
    EventDetector detector(100);

    // Record events with different priorities
    SimulationEvent ev1;
    ev1.tick = 10;
    ev1.type = SimulationEventType::ForageSuccess;
    ev1.priority = 40.0;
    ev1.description = "Forage";
    detector.record_event(ev1);

    SimulationEvent ev2;
    ev2.tick = 12;
    ev2.type = SimulationEventType::LessonTaught;
    ev2.priority = 85.0;
    ev2.description = "God Fly Lesson";
    detector.record_event(ev2);

    SimulationEvent ev3;
    ev3.tick = 14;
    ev3.type = SimulationEventType::Collision;
    ev3.priority = 60.0;
    ev3.description = "Collision";
    detector.record_event(ev3);

    // Highest priority must be LessonTaught (85.0)
    const SimulationEvent* top = detector.highest_priority_event();
    ALWAYS_ASSERT(top != nullptr);
    ALWAYS_ASSERT(top->type == SimulationEventType::LessonTaught);
    ALWAYS_ASSERT(top->priority == 85.0);

    // Equal priority tie-breaking: earlier tick or lower ID
    SimulationEvent ev4;
    ev4.tick = 15;
    ev4.type = SimulationEventType::BrainSurge;
    ev4.priority = 85.0; // Same priority as ev2
    ev4.description = "Brain Surge";
    detector.record_event(ev4);

    // ev2 was recorded at tick 12 with ID 2, ev4 at tick 15 with ID 4.
    // Deterministic tie-breaking rules: earlier tick (12 < 15) must win!
    const SimulationEvent* top_after_tie = detector.highest_priority_event();
    ALWAYS_ASSERT(top_after_tie != nullptr);
    ALWAYS_ASSERT(top_after_tie->id == 2);
    ALWAYS_ASSERT(top_after_tie->type == SimulationEventType::LessonTaught);

    std::cout << "  -> Priority and deterministic tie-breaking passed." << std::endl;
}

void test_event_lifecycle_and_disappearance() {
    std::cout << "[Test] Running test_event_lifecycle_and_disappearance..." << std::endl;
    EventDetector detector(100);

    SimulationEvent short_ev;
    short_ev.tick = 1;
    short_ev.type = SimulationEventType::Collision;
    short_ev.priority = 95.0;
    short_ev.lifetime_ticks = 5;
    detector.record_event(short_ev);

    SimulationEvent persistent_ev;
    persistent_ev.tick = 2;
    persistent_ev.type = SimulationEventType::WeatherShift;
    persistent_ev.priority = 30.0;
    persistent_ev.lifetime_ticks = 50;
    detector.record_event(persistent_ev);

    ALWAYS_ASSERT(detector.highest_priority_event()->priority == 95.0);

    // Step 5 ticks: short_ev should reach lifetime and expire
    for (uint64_t t = 1; t <= 5; ++t) {
        detector.step(t);
    }

    // Now persistent_ev must automatically become highest priority
    const SimulationEvent* next_top = detector.highest_priority_event();
    ALWAYS_ASSERT(next_top != nullptr);
    ALWAYS_ASSERT(next_top->type == SimulationEventType::WeatherShift);
    ALWAYS_ASSERT(next_top->priority == 30.0);

    std::cout << "  -> Event lifecycle and expiration fallback passed." << std::endl;
}

void test_camera_cooldown_and_hysteresis() {
    std::cout << "[Test] Running test_camera_cooldown_and_hysteresis..." << std::endl;
    CameraDirectorConfig cfg;
    cfg.min_shot_duration_ticks = 30;
    cfg.cooldown_ticks = 15;
    cfg.interrupt_priority_hysteresis = 25.0;

    SingleCameraDirector cam(CameraChannel::Cam3_Event, cfg);

    CameraTarget initial_target;
    initial_target.position = Vec3{10.0, 2.0, 10.0};
    initial_target.priority = 40.0;
    cam.assign_target(initial_target, ShotType::Medium);

    // Immediately try to switch to a marginally higher priority target (50.0)
    CameraTarget marginal_target;
    marginal_target.position = Vec3{20.0, 2.0, 20.0};
    marginal_target.priority = 50.0; // Difference is 10, below hysteresis of 25

    ALWAYS_ASSERT(!cam.can_switch_target(marginal_target)); // Must be rejected due to min shot duration & hysteresis!

    // High priority target (70.0) where diff (30.0) > hysteresis (25.0)
    CameraTarget urgent_target;
    urgent_target.position = Vec3{30.0, 2.0, 30.0};
    urgent_target.priority = 70.0;

    ALWAYS_ASSERT(cam.can_switch_target(urgent_target)); // Allowed to interrupt!
    cam.assign_target(urgent_target, ShotType::Close);
    ALWAYS_ASSERT(cam.current_target().priority == 70.0);
    ALWAYS_ASSERT(cam.cooldown_remaining_ticks() == 15);

    // During cooldown, a non-urgent target cannot switch
    ALWAYS_ASSERT(!cam.can_switch_target(marginal_target));

    std::cout << "  -> Camera cooldown and interruption hysteresis passed." << std::endl;
}

void test_camera_transition_numerical_stability() {
    std::cout << "[Test] Running test_camera_transition_numerical_stability..." << std::endl;
    CameraDirectorConfig cfg;
    cfg.transition_speed = 5.0;

    SingleCameraDirector cam(CameraChannel::Cam1_GodFly, cfg);

    CameraTarget t1;
    t1.position = Vec3{0.0, 0.0, 0.0};
    cam.assign_target(t1, ShotType::Medium, /*force=*/true);

    // Initial step to settle
    for (int i = 0; i < 60; ++i) {
        cam.step(1.0 / 60.0, i, i / 60.0, nullptr);
    }
    Vec3 p1 = cam.current_pose().position;
    ALWAYS_ASSERT(std::isfinite(p1.x) && std::isfinite(p1.y) && std::isfinite(p1.z));

    // Now switch to distant target (100m away)
    CameraTarget t2;
    t2.position = Vec3{100.0, 10.0, 100.0};
    cam.assign_target(t2, ShotType::Wide, /*force=*/true);

    // Step across 120 frames (2 seconds)
    Vec3 prev_p = p1;
    for (int i = 60; i < 180; ++i) {
        cam.step(1.0 / 60.0, i, i / 60.0, nullptr);
        Vec3 curr_p = cam.current_pose().position;

        // Verify finite and no sudden jumps (teleports)
        ALWAYS_ASSERT(std::isfinite(curr_p.x) && std::isfinite(curr_p.y) && std::isfinite(curr_p.z));
        double frame_delta = (curr_p - prev_p).length();
        ALWAYS_ASSERT(frame_delta < 50.0); // Smooth continuous movement, no teleportation
        prev_p = curr_p;
    }

    // Should be close to target
    Vec3 diff = cam.current_pose().look_at - t2.position;
    ALWAYS_ASSERT(diff.length() < 0.1);

    std::cout << "  -> Camera transition numerical stability passed." << std::endl;
}

void test_camera_director_serialization_and_replay() {
    std::cout << "[Test] Running test_camera_director_serialization_and_replay..." << std::endl;
    CameraDirector director;

    EventDetector events;
    SimulationEvent ev;
    ev.tick = 1;
    ev.priority = 80.0;
    ev.position = Vec3{15.0, 2.0, 15.0};
    ev.description = "High Value Event";
    events.record_event(ev);

    AgentManager agents;
    llm::ModelManager model_mgr;
    llm::GodFly god_fly(&model_mgr);

    // Step director 30 ticks
    for (uint64_t t = 1; t <= 30; ++t) {
        director.step(1.0 / 60.0, t, t / 60.0, events, nullptr, agents, &god_fly);
    }

    // Serialize state
    nlohmann::json snapshot = director.to_json();

    // Create new director and restore state
    CameraDirector restored_director;
    restored_director.from_json(snapshot);

    // Verify all 4 channels match
    for (uint32_t c = 0; c < 4; ++c) {
        CameraChannel ch = static_cast<CameraChannel>(c);
        const auto& orig_p = director.channel(ch).current_pose();
        const auto& rest_p = restored_director.channel(ch).current_pose();

        ALWAYS_ASSERT(std::abs(orig_p.position.x - rest_p.position.x) < 1e-5);
        ALWAYS_ASSERT(std::abs(orig_p.position.y - rest_p.position.y) < 1e-5);
        ALWAYS_ASSERT(std::abs(orig_p.position.z - rest_p.position.z) < 1e-5);
        ALWAYS_ASSERT(director.channel(ch).current_shot() == restored_director.channel(ch).current_shot());
    }

    std::cout << "  -> Camera director serialization and replay match passed." << std::endl;
}

int main() {
    std::cout << "=== FLGODTV STAGE 18 CAMERA DIRECTOR TEST SUITE ===" << std::endl;
    test_event_detector_priority_and_tie_breaking();
    test_event_lifecycle_and_disappearance();
    test_camera_cooldown_and_hysteresis();
    test_camera_transition_numerical_stability();
    test_camera_director_serialization_and_replay();
    std::cout << "=== ALL STAGE 18 CAMERA DIRECTOR TESTS PASSED ===" << std::endl;
    return 0;
}
