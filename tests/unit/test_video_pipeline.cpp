#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include "flgod/video/video_types.hpp"

using namespace flgod;
using namespace flgod::video;

int main() {
    std::cout << "[VideoPipelineTest] Starting unit tests for Video Pipeline Subsystem..." << std::endl;

    // 1. Resolution & Config validation
    VideoResolution res{1280, 720};
    assert(res.is_valid());
    VideoResolution invalid_res{1281, 720}; // Odd width is invalid for yuv420p
    assert(!invalid_res.is_valid());

    VideoConfig config;
    config.resolution = res;
    config.fps = 30;
    config.duration_seconds = 3.0;
    assert(config.expected_frames() == 90);

    // 2. Event Scoring & Ranking
    std::vector<SimulationEvent> events;

    SimulationEvent ev1;
    ev1.id = 1;
    ev1.type = SimulationEventType::Generic;
    ev1.priority = 20.0;
    ev1.description = "General agent activity";
    events.push_back(ev1);

    SimulationEvent ev2;
    ev2.id = 2;
    ev2.type = SimulationEventType::SpeciationDivergence;
    ev2.priority = 95.0;
    ev2.description = "Lineage Speciation Divergence Detected";
    events.push_back(ev2);

    SimulationEvent ev3;
    ev3.id = 3;
    ev3.type = SimulationEventType::LessonTaught;
    ev3.priority = 85.0;
    ev3.description = "God Fly Instructed Student on Foraging";
    events.push_back(ev3);

    auto ranked = EventRanker::rank_events(events);
    assert(ranked.size() == 3);
    assert(ranked[0].event_id == 2); // Speciation should rank highest
    assert(ranked[0].type == SimulationEventType::SpeciationDivergence);
    assert(ranked[0].composite_score > ranked[1].composite_score);
    assert(ranked[1].event_id == 3); // LessonTaught should rank second
    assert(ranked[2].event_id == 1); // Generic should rank lowest

    // 3. Shot Planning
    auto plan = ShotPlan::create_cinematic_plan(90, &ev2);
    assert(plan.is_valid());
    assert(plan.segments.size() == 3);
    assert(plan.segments[0].start_frame == 0);
    assert(plan.segments[0].end_frame == 30);
    assert(plan.segments[1].start_frame == 30);
    assert(plan.segments[1].end_frame == 60);
    assert(plan.segments[1].channel == CameraChannel::Cam3_Event);
    assert(plan.segments[2].start_frame == 60);
    assert(plan.segments[2].end_frame == 90);
    assert(plan.segments[2].channel == CameraChannel::Cam1_GodFly);

    // Test invalid shot plan detection (gap)
    ShotPlan invalid_plan;
    invalid_plan.total_frames = 90;
    invalid_plan.segments.push_back({0, 30, CameraChannel::Cam4_EnvironmentColony, ShotType::Establishing, NULL_ENTITY, "Seg1"});
    invalid_plan.segments.push_back({35, 90, CameraChannel::Cam1_GodFly, ShotType::Orbit, NULL_ENTITY, "Seg2"}); // Gap 30->35
    assert(!invalid_plan.is_valid());

    // 4. Render Manifest Serialization Roundtrip
    RenderManifest manifest;
    manifest.simulation_seed = 42;
    manifest.simulation_version = "0.1.0";
    manifest.start_tick = 0;
    manifest.end_tick = 60;
    manifest.resolution = {1280, 720};
    manifest.fps = 30;
    manifest.expected_frames = 90;
    manifest.produced_frames = 90;
    manifest.output_path = "videos/flgodtv_cinematic_highlight.mp4";
    manifest.output_sha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    manifest.output_file_size = 1048576;
    manifest.duration_seconds = 3.0;
    manifest.validation_result = "PASS";
    manifest.shots = {
        "Shot 1: Environment & Colony Horizon Establishing Shot (Frames 0-30)",
        "Shot 2: Speciation Event Close Tracking (Frames 30-60)",
        "Shot 3: God Fly Orbital Guidance (Frames 60-90)"
    };

    nlohmann::json manifest_json = manifest.to_json();
    assert(manifest_json["simulation_seed"] == 42);
    assert(manifest_json["expected_frames"] == 90);
    assert(manifest_json["validation_result"] == "PASS");

    RenderManifest parsed = RenderManifest::from_json(manifest_json);
    assert(parsed.simulation_seed == 42);
    assert(parsed.expected_frames == 90);
    assert(parsed.resolution.width == 1280);
    assert(parsed.resolution.height == 720);
    assert(parsed.output_sha256 == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(parsed.shots.size() == 3);

    std::cout << "[VideoPipelineTest] ALL VIDEO PIPELINE UNIT TESTS PASSED (100%)!" << std::endl;
    return 0;
}
