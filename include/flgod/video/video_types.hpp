#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include "flgod/core/version.hpp"
#include "flgod/camera/camera_types.hpp"
#include "flgod/camera/event_detector.hpp"

namespace flgod::video {

struct VideoResolution {
    uint32_t width{1280};
    uint32_t height{720};

    [[nodiscard]] bool is_valid() const noexcept {
        return width > 0 && height > 0 && (width % 2 == 0) && (height % 2 == 0);
    }
};

struct VideoConfig {
    VideoResolution resolution{1280, 720};
    uint32_t fps{30};
    double duration_seconds{3.0};
    std::string codec{"libx264"};
    std::string pixel_format{"yuv420p"};
    int crf{22};
    std::string output_path{"videos/flgodtv_cinematic_highlight.mp4"};

    [[nodiscard]] uint32_t expected_frames() const noexcept {
        return static_cast<uint32_t>(std::round(fps * duration_seconds));
    }
};

struct EventScore {
    uint64_t event_id{0};
    flgod::SimulationEventType type{flgod::SimulationEventType::Generic};
    double priority{0.0};
    double novelty{1.0};
    double causal_importance{1.0};
    double composite_score{0.0};

    static EventScore calculate(const flgod::SimulationEvent& ev, double novelty_weight = 0.4, double causal_weight = 0.6) {
        EventScore score;
        score.event_id = ev.id;
        score.type = ev.type;
        score.priority = ev.priority;

        // Domain-specific scoring per GEMINI.md Section 89
        switch (ev.type) {
            case flgod::SimulationEventType::SpeciationDivergence:
            case flgod::SimulationEventType::TechExecution:
                score.novelty = 1.0;
                score.causal_importance = 1.0;
                break;
            case flgod::SimulationEventType::LessonTaught:
            case flgod::SimulationEventType::Predation:
                score.novelty = 0.8;
                score.causal_importance = 0.85;
                break;
            case flgod::SimulationEventType::Construction:
            case flgod::SimulationEventType::Migration:
                score.novelty = 0.7;
                score.causal_importance = 0.75;
                break;
            default:
                score.novelty = 0.5;
                score.causal_importance = 0.5;
                break;
        }

        score.composite_score = (ev.priority * 0.5) + (score.novelty * 100.0 * novelty_weight) + (score.causal_importance * 100.0 * causal_weight);
        return score;
    }
};

struct ShotSegment {
    uint32_t start_frame{0};
    uint32_t end_frame{0};
    flgod::CameraChannel channel{flgod::CameraChannel::Cam1_GodFly};
    flgod::ShotType shot_type{flgod::ShotType::Orbit};
    flgod::EntityID target_entity{NULL_ENTITY};
    std::string description;

    [[nodiscard]] uint32_t duration_frames() const noexcept {
        return (end_frame >= start_frame) ? (end_frame - start_frame) : 0;
    }
};

struct ShotPlan {
    uint32_t total_frames{90};
    std::vector<ShotSegment> segments;

    [[nodiscard]] bool is_valid() const noexcept {
        if (segments.empty()) return false;
        uint32_t current = 0;
        for (const auto& seg : segments) {
            if (seg.start_frame != current || seg.end_frame <= seg.start_frame) {
                return false;
            }
            current = seg.end_frame;
        }
        return current == total_frames;
    }

    static ShotPlan create_cinematic_plan(uint32_t total_frames, const flgod::SimulationEvent* best_event = nullptr) {
        ShotPlan plan;
        plan.total_frames = total_frames;

        uint32_t seg1_frames = total_frames / 3;
        uint32_t seg2_frames = total_frames / 3;

        // Shot 1: Wide Establishing
        plan.segments.push_back({
            0,
            seg1_frames,
            flgod::CameraChannel::Cam4_EnvironmentColony,
            flgod::ShotType::Establishing,
            NULL_ENTITY,
            "Environment & Colony Horizon Establishing Shot"
        });

        // Shot 2: Best Event Focus / Agent Tracking
        flgod::CameraChannel focus_chan = best_event ? flgod::CameraChannel::Cam3_Event : flgod::CameraChannel::Cam2_LearningAgent;
        flgod::ShotType focus_shot = best_event ? flgod::ShotType::Close : flgod::ShotType::Tracking;
        flgod::EntityID focus_ent = best_event ? best_event->source_entity : flgod::EntityID(1);
        std::string desc = best_event ? best_event->description : "Agent Aerial Maneuver";

        plan.segments.push_back({
            seg1_frames,
            seg1_frames + seg2_frames,
            focus_chan,
            focus_shot,
            focus_ent,
            desc
        });

        // Shot 3: God Fly Orbit / Guidance
        plan.segments.push_back({
            seg1_frames + seg2_frames,
            total_frames,
            flgod::CameraChannel::Cam1_GodFly,
            flgod::ShotType::Orbit,
            flgod::EntityID(1000000000000ULL),
            "God Fly Orbital Guidance"
        });

        return plan;
    }
};

struct FrameValidationResult {
    uint32_t total_expected{0};
    uint32_t valid_frames{0};
    uint32_t missing_frames{0};
    uint32_t corrupted_frames{0};
    bool is_valid{false};
    std::string error_message;
};

struct RenderManifest {
    uint64_t simulation_seed{1337};
    std::string simulation_version{"0.1.0"};
    uint64_t start_tick{0};
    uint64_t end_tick{60};
    VideoResolution resolution{1280, 720};
    uint32_t fps{30};
    uint32_t expected_frames{90};
    uint32_t produced_frames{90};
    std::string output_path{"videos/flgodtv_cinematic_highlight.mp4"};
    std::string output_sha256;
    uint64_t output_file_size{0};
    double duration_seconds{3.0};
    std::string validation_result{"PASS"};
    std::vector<std::string> shots;

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["simulation_seed"] = simulation_seed;
        j["simulation_version"] = simulation_version;
        j["start_tick"] = start_tick;
        j["end_tick"] = end_tick;
        j["resolution"] = {{"width", resolution.width}, {"height", resolution.height}};
        j["fps"] = fps;
        j["expected_frames"] = expected_frames;
        j["produced_frames"] = produced_frames;
        j["output_path"] = output_path;
        j["output_sha256"] = output_sha256;
        j["output_file_size"] = output_file_size;
        j["duration_seconds"] = duration_seconds;
        j["validation_result"] = validation_result;
        j["shots"] = shots;
        return j;
    }

    static RenderManifest from_json(const nlohmann::json& j) {
        RenderManifest m;
        m.simulation_seed = j.value("simulation_seed", 1337ULL);
        m.simulation_version = j.value("simulation_version", "0.1.0");
        m.start_tick = j.value("start_tick", 0ULL);
        m.end_tick = j.value("end_tick", 60ULL);
        if (j.contains("resolution")) {
            m.resolution.width = j["resolution"].value("width", 1280U);
            m.resolution.height = j["resolution"].value("height", 720U);
        }
        m.fps = j.value("fps", 30U);
        m.expected_frames = j.value("expected_frames", 90U);
        m.produced_frames = j.value("produced_frames", 90U);
        m.output_path = j.value("output_path", "");
        m.output_sha256 = j.value("output_sha256", "");
        m.output_file_size = j.value("output_file_size", 0ULL);
        m.duration_seconds = j.value("duration_seconds", 3.0);
        m.validation_result = j.value("validation_result", "UNKNOWN");
        if (j.contains("shots") && j["shots"].is_array()) {
            m.shots = j["shots"].get<std::vector<std::string>>();
        }
        return m;
    }
};

class EventRanker {
public:
    static std::vector<EventScore> rank_events(const std::vector<flgod::SimulationEvent>& events) {
        std::vector<EventScore> scores;
        scores.reserve(events.size());
        for (const auto& ev : events) {
            scores.push_back(EventScore::calculate(ev));
        }

        std::sort(scores.begin(), scores.end(), [](const EventScore& a, const EventScore& b) {
            if (std::abs(a.composite_score - b.composite_score) > 1e-4) {
                return a.composite_score > b.composite_score;
            }
            return a.event_id < b.event_id;
        });

        return scores;
    }
};

} // namespace flgod::video
