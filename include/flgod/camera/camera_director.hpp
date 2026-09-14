#pragma once

#include "flgod/camera/camera_types.hpp"
#include "flgod/camera/event_detector.hpp"
#include "flgod/world/world.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/llm/god_fly.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace flgod {

struct CameraDirectorConfig {
    uint32_t min_shot_duration_ticks{60};       // 1.0s at 60Hz
    uint32_t cooldown_ticks{30};                // 0.5s cooldown after switch
    double interrupt_priority_hysteresis{20.0}; // Needs 20+ higher priority to interrupt
    double transition_speed{4.0};               // Smooth lerp speed factor
    double min_height_above_ground{0.5};        // Collision avoidance clearance
};

class SingleCameraDirector {
public:
    explicit SingleCameraDirector(CameraChannel channel, CameraDirectorConfig config = {})
        : m_channel(channel), m_config(config) {}

    [[nodiscard]] CameraChannel channel() const noexcept { return m_channel; }
    [[nodiscard]] const CameraPose& current_pose() const noexcept { return m_current_pose; }
    [[nodiscard]] const CameraPose& target_pose() const noexcept { return m_target_pose; }
    [[nodiscard]] const CameraTarget& current_target() const noexcept { return m_current_target; }
    [[nodiscard]] ShotType current_shot() const noexcept { return m_current_shot; }
    [[nodiscard]] uint32_t shot_elapsed_ticks() const noexcept { return m_shot_elapsed_ticks; }
    [[nodiscard]] uint32_t cooldown_remaining_ticks() const noexcept { return m_cooldown_remaining_ticks; }
    [[nodiscard]] bool is_locked() const noexcept { return m_is_locked; }
    void set_locked(bool locked) noexcept { m_is_locked = locked; }

    [[nodiscard]] bool can_switch_target(const CameraTarget& new_target) const noexcept {
        if (m_is_locked) return false;

        // If current target is empty or invalid, switch is always allowed
        if (!m_current_target.is_valid) {
            return true;
        }

        // Higher priority interrupt can bypass min shot duration if difference exceeds hysteresis
        if (new_target.priority > m_current_target.priority + m_config.interrupt_priority_hysteresis) {
            return true;
        }

        // Must respect minimum shot duration
        if (m_shot_elapsed_ticks < m_config.min_shot_duration_ticks) {
            return false;
        }

        // Must respect cooldown
        if (m_cooldown_remaining_ticks > 0) {
            return false;
        }

        return true;
    }

    void assign_target(CameraTarget target, ShotType shot, bool force = false) {
        if (!force && !can_switch_target(target)) {
            return;
        }
        target.is_valid = true;
        m_current_target = target;
        m_current_shot = shot;
        m_shot_elapsed_ticks = 0;
        m_cooldown_remaining_ticks = m_config.cooldown_ticks;
    }

    void compute_shot_pose(const CameraTarget& target, ShotType shot, double time_sec, World* world, CameraPose& out_pose) const {
        Vec3 focus = target.position;
        out_pose.look_at = focus;
        out_pose.up = Vec3{0.0, 1.0, 0.0};

        switch (shot) {
            case ShotType::Macro: {
                out_pose.distance = 1.2f;
                out_pose.fov = 35.0f;
                out_pose.position = focus + Vec3{0.8, 0.4, 0.8};
                break;
            }
            case ShotType::Close: {
                out_pose.distance = 3.0f;
                out_pose.fov = 45.0f;
                out_pose.position = focus + Vec3{1.8, 1.2, 2.0};
                break;
            }
            case ShotType::Medium: {
                out_pose.distance = 6.0f;
                out_pose.fov = 55.0f;
                out_pose.position = focus + Vec3{3.5, 2.5, 4.0};
                break;
            }
            case ShotType::Wide: {
                out_pose.distance = 18.0f;
                out_pose.fov = 65.0f;
                out_pose.position = focus + Vec3{10.0, 8.0, 12.0};
                break;
            }
            case ShotType::Establishing: {
                out_pose.distance = 50.0f;
                out_pose.fov = 75.0f;
                out_pose.position = focus + Vec3{25.0, 25.0, 35.0};
                break;
            }
            case ShotType::Tracking: {
                out_pose.distance = 4.5f;
                out_pose.fov = 50.0f;
                // Follow behind velocity vector with slight offset
                Vec3 lead = target.velocity * 0.5;
                out_pose.look_at = focus + lead;
                out_pose.position = focus - target.velocity * 0.8 + Vec3{2.0, 1.8, 3.0};
                break;
            }
            case ShotType::Orbit: {
                out_pose.distance = 7.0f;
                out_pose.fov = 55.0f;
                double angle = time_sec * 0.5;
                double rad = 7.0;
                out_pose.position = focus + Vec3{rad * std::cos(angle), 3.0, rad * std::sin(angle)};
                break;
            }
            case ShotType::Overhead: {
                out_pose.distance = 15.0f;
                out_pose.fov = 60.0f;
                out_pose.position = focus + Vec3{0.0, 15.0, 0.1}; // slight Z epsilon for stable UP vector
                break;
            }
            case ShotType::LowAngle: {
                out_pose.distance = 2.5f;
                out_pose.fov = 50.0f;
                out_pose.position = focus + Vec3{1.5, 0.3, 1.8};
                break;
            }
            case ShotType::POV: {
                out_pose.distance = 0.5f;
                out_pose.fov = 70.0f;
                double vlen = target.velocity.length();
                Vec3 fwd = vlen > 0.01 ? target.velocity * (1.0 / vlen) : Vec3{0.0, 0.0, 1.0};
                out_pose.position = focus + Vec3{0.0, 0.1, 0.0};
                out_pose.look_at = focus + fwd * 5.0;
                break;
            }
            case ShotType::ReactionShot: {
                out_pose.distance = 3.5f;
                out_pose.fov = 48.0f;
                out_pose.position = focus + Vec3{-2.0, 1.2, 2.5};
                break;
            }
        }

        // Avoid collision with terrain if world is provided
        if (world != nullptr) {
            double ground_y = world->sample_elevation(out_pose.position.x, out_pose.position.z);
            double min_y = ground_y + m_config.min_height_above_ground;
            if (out_pose.position.y < min_y) {
                out_pose.position.y = min_y;
            }
        }
    }

    void step(double dt, uint64_t /*tick*/, double current_time_sec, World* world) {
        m_shot_elapsed_ticks++;
        if (m_cooldown_remaining_ticks > 0) {
            m_cooldown_remaining_ticks--;
        }

        // Compute desired target pose
        compute_shot_pose(m_current_target, m_current_shot, current_time_sec, world, m_target_pose);

        // Smooth transition (interpolation) towards target pose
        double factor = std::clamp(dt * m_config.transition_speed, 0.0, 1.0);
        
        m_current_pose.position = m_current_pose.position + (m_target_pose.position - m_current_pose.position) * factor;
        m_current_pose.look_at = m_current_pose.look_at + (m_target_pose.look_at - m_current_pose.look_at) * factor;
        m_current_pose.up = m_current_pose.up + (m_target_pose.up - m_current_pose.up) * factor;
        m_current_pose.fov = static_cast<float>(m_current_pose.fov + (m_target_pose.fov - m_current_pose.fov) * factor);
        m_current_pose.distance = static_cast<float>(m_current_pose.distance + (m_target_pose.distance - m_current_pose.distance) * factor);

        // Ensure terrain clearance on interpolated position as well
        if (world != nullptr) {
            double ground_y = world->sample_elevation(m_current_pose.position.x, m_current_pose.position.z);
            double min_y = ground_y + m_config.min_height_above_ground;
            if (m_current_pose.position.y < min_y) {
                m_current_pose.position.y = min_y;
            }
        }
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"channel", static_cast<uint32_t>(m_channel)},
            {"channel_name", to_string(m_channel)},
            {"current_pose", m_current_pose.to_json()},
            {"target_pose", m_target_pose.to_json()},
            {"target", m_current_target.to_json()},
            {"shot", static_cast<uint32_t>(m_current_shot)},
            {"shot_name", to_string(m_current_shot)},
            {"elapsed_ticks", m_shot_elapsed_ticks},
            {"cooldown_ticks", m_cooldown_remaining_ticks},
            {"locked", m_is_locked}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("current_pose")) m_current_pose = CameraPose::from_json(j["current_pose"]);
        if (j.contains("target_pose")) m_target_pose = CameraPose::from_json(j["target_pose"]);
        if (j.contains("target")) m_current_target = CameraTarget::from_json(j["target"]);
        m_current_shot = static_cast<ShotType>(j.value("shot", static_cast<uint32_t>(ShotType::Medium)));
        m_shot_elapsed_ticks = j.value("elapsed_ticks", 0U);
        m_cooldown_remaining_ticks = j.value("cooldown_ticks", 0U);
        m_is_locked = j.value("locked", false);
    }

private:
    CameraChannel m_channel;
    CameraDirectorConfig m_config;
    CameraPose m_current_pose{};
    CameraPose m_target_pose{};
    CameraTarget m_current_target{};
    ShotType m_current_shot{ShotType::Medium};
    uint32_t m_shot_elapsed_ticks{0};
    uint32_t m_cooldown_remaining_ticks{0};
    bool m_is_locked{false};
};

class CameraDirector {
public:
    explicit CameraDirector(CameraDirectorConfig config = {})
        : m_config(config),
          m_channels{
              SingleCameraDirector(CameraChannel::Cam1_GodFly, config),
              SingleCameraDirector(CameraChannel::Cam2_LearningAgent, config),
              SingleCameraDirector(CameraChannel::Cam3_Event, config),
              SingleCameraDirector(CameraChannel::Cam4_EnvironmentColony, config)
          } {}

    [[nodiscard]] const SingleCameraDirector& channel(CameraChannel ch) const {
        return m_channels[static_cast<size_t>(ch)];
    }

    [[nodiscard]] SingleCameraDirector& channel(CameraChannel ch) {
        return m_channels[static_cast<size_t>(ch)];
    }

    void step(double dt, uint64_t tick, double current_time_sec,
              const EventDetector& event_detector,
              World* world,
              const AgentManager& agent_mgr,
              const llm::GodFly* god_fly) {
        
        // 1. Channel 1: God Fly Director
        if (god_fly != nullptr) {
            CameraTarget gf_target;
            gf_target.entity_id = EntityID(1000000000000ULL);
            gf_target.position = god_fly->position();
            gf_target.velocity = Vec3{0.0, 0.0, 0.0};
            gf_target.bounding_radius = 0.5f;
            gf_target.priority = 90.0;
            gf_target.label = "God Fly";
            gf_target.suggested_shot = ShotType::Orbit;
            m_channels[0].assign_target(gf_target, ShotType::Orbit);
        }

        // 2. Channel 2: Learning Agent Director (select agent with highest activity / learning experience)
        if (agent_mgr.agent_count() > 0) {
            EntityID best_agent_id{NULL_ENTITY};
            double best_score = -1.0;
            Vec3 best_pos{0.0, 0.0, 0.0};
            Vec3 best_vel{0.0, 0.0, 0.0};

            // Deterministic selection over sorted agent IDs
            for (const auto& [id, agent] : agent_mgr.agents()) {
                if (!agent.is_alive()) continue;
                double score = agent.velocity().length() + (agent.drives().hunger * 0.1);
                if (score > best_score) {
                    best_score = score;
                    best_agent_id = id;
                    best_pos = agent.position();
                    best_vel = agent.velocity();
                }
            }

            if (best_agent_id != NULL_ENTITY) {
                CameraTarget la_target;
                la_target.entity_id = best_agent_id;
                la_target.position = best_pos;
                la_target.velocity = best_vel;
                la_target.bounding_radius = 0.3f;
                la_target.priority = 50.0;
                la_target.label = "Learning Agent " + std::to_string(best_agent_id.index());
                la_target.suggested_shot = ShotType::Tracking;
                m_channels[1].assign_target(la_target, ShotType::Tracking);
            }
        }

        // 3. Channel 3: Cinematic Event Director (evaluates active events deterministically)
        const SimulationEvent* top_event = event_detector.highest_priority_event();
        if (top_event != nullptr) {
            CameraTarget ev_target;
            ev_target.source_event_id = top_event->id;
            ev_target.entity_id = top_event->source_entity;
            ev_target.position = top_event->position;
            ev_target.priority = top_event->priority;
            ev_target.label = top_event->description;

            // Choose appropriate shot type based on event type
            ShotType chosen_shot = ShotType::Medium;
            switch (top_event->type) {
                case SimulationEventType::Collision: chosen_shot = ShotType::Close; break;
                case SimulationEventType::LessonTaught: chosen_shot = ShotType::Medium; break;
                case SimulationEventType::TechExecution: chosen_shot = ShotType::Macro; break;
                case SimulationEventType::WeatherShift: chosen_shot = ShotType::Wide; break;
                case SimulationEventType::SpeciationDivergence: chosen_shot = ShotType::Establishing; break;
                default: chosen_shot = ShotType::Medium; break;
            }
            ev_target.suggested_shot = chosen_shot;
            m_channels[2].assign_target(ev_target, chosen_shot);
        } else {
            // Fallback when no active event exists: focus on world center or active agent
            if (m_channels[2].current_target().source_event_id != 0 &&
                m_channels[2].shot_elapsed_ticks() >= m_config.min_shot_duration_ticks) {
                CameraTarget fallback_target;
                fallback_target.position = Vec3{30.0, 5.0, 30.0};
                fallback_target.priority = 10.0;
                fallback_target.label = "World Overview";
                fallback_target.suggested_shot = ShotType::Wide;
                m_channels[2].assign_target(fallback_target, ShotType::Wide, /*force=*/true);
            }
        }

        // 4. Channel 4: Environment & Colony Director
        CameraTarget colony_target;
        if (!agent_mgr.colonies().empty()) {
            const auto& first_colony = agent_mgr.colonies().begin()->second;
            colony_target.position = first_colony.nest_position();
            colony_target.bounding_radius = static_cast<float>(first_colony.territory_radius());
            colony_target.priority = 40.0;
            colony_target.label = "Colony Nest";
            colony_target.suggested_shot = ShotType::Wide;
        } else {
            colony_target.position = Vec3{30.0, 0.0, 30.0};
            colony_target.bounding_radius = 50.0f;
            colony_target.priority = 20.0;
            colony_target.label = "Environment";
            colony_target.suggested_shot = ShotType::Establishing;
        }
        m_channels[3].assign_target(colony_target, ShotType::Establishing);

        // Step all 4 single camera directors
        for (auto& ch_director : m_channels) {
            ch_director.step(dt, tick, current_time_sec, world);
        }
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json channels_json = nlohmann::json::array();
        for (const auto& ch : m_channels) {
            channels_json.push_back(ch.to_json());
        }
        return {
            {"channels", channels_json}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("channels") && j["channels"].is_array()) {
            size_t idx = 0;
            for (const auto& ch_json : j["channels"]) {
                if (idx < m_channels.size()) {
                    m_channels[idx].from_json(ch_json);
                    idx++;
                }
            }
        }
    }

private:
    CameraDirectorConfig m_config;
    std::array<SingleCameraDirector, 4> m_channels;
};

} // namespace flgod
