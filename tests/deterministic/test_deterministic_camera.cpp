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

void test_deterministic_camera_decisions() {
    std::cout << "[Test] Running test_deterministic_camera_decisions across runs..." << std::endl;

    auto run_simulation = [](uint64_t ticks) -> std::vector<CameraPose> {
        CameraDirector director;
        EventDetector detector;

        // Schedule events deterministically
        SimulationEvent e1;
        e1.tick = 5;
        e1.type = SimulationEventType::ForageSuccess;
        e1.priority = 45.0;
        e1.position = Vec3{12.0, 1.0, 8.0};
        detector.record_event(e1);

        SimulationEvent e2;
        e2.tick = 20;
        e2.type = SimulationEventType::LessonTaught;
        e2.priority = 88.0;
        e2.position = Vec3{25.0, 4.0, 20.0};
        detector.record_event(e2);

        SimulationEvent e3;
        e3.tick = 50;
        e3.type = SimulationEventType::TechExecution;
        e3.priority = 75.0;
        e3.position = Vec3{5.0, 0.5, 30.0};
        detector.record_event(e3);

        AgentManager agents;
        llm::ModelManager model_mgr;
        llm::GodFly god_fly(&model_mgr);

        std::vector<CameraPose> history;
        history.reserve(ticks);

        for (uint64_t t = 1; t <= ticks; ++t) {
            detector.step(t);
            director.step(1.0 / 60.0, t, t / 60.0, detector, nullptr, agents, &god_fly);
            // Record event camera (Channel 3 / Cam3_Event) pose
            history.push_back(director.channel(CameraChannel::Cam3_Event).current_pose());
        }

        return history;
    };

    auto run1 = run_simulation(150);
    auto run2 = run_simulation(150);

    ALWAYS_ASSERT(run1.size() == run2.size());
    for (size_t i = 0; i < run1.size(); ++i) {
        ALWAYS_ASSERT(run1[i].position.x == run2[i].position.x);
        ALWAYS_ASSERT(run1[i].position.y == run2[i].position.y);
        ALWAYS_ASSERT(run1[i].position.z == run2[i].position.z);
        ALWAYS_ASSERT(run1[i].look_at.x == run2[i].look_at.x);
        ALWAYS_ASSERT(run1[i].look_at.y == run2[i].look_at.y);
        ALWAYS_ASSERT(run1[i].look_at.z == run2[i].look_at.z);
        ALWAYS_ASSERT(run1[i].fov == run2[i].fov);
    }

    std::cout << "  -> Bit-exact deterministic camera decisions and poses verified over 150 ticks." << std::endl;
}

int main() {
    std::cout << "=== FLGODTV STAGE 18 DETERMINISTIC CAMERA TEST ===" << std::endl;
    test_deterministic_camera_decisions();
    std::cout << "=== ALL DETERMINISTIC CAMERA TESTS PASSED ===" << std::endl;
    return 0;
}
