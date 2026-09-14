#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "flgod/core/version.hpp"
#include "flgod/core/entity_id.hpp"

namespace flgod::integration {

inline constexpr uint32_t PROTOCOL_VERSION_MAJOR = 1;
inline constexpr uint32_t PROTOCOL_VERSION_MINOR = 0;

struct ProtocolVersion {
    uint32_t major{PROTOCOL_VERSION_MAJOR};
    uint32_t minor{PROTOCOL_VERSION_MINOR};

    [[nodiscard]] std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor);
    }

    [[nodiscard]] bool is_compatible(const ProtocolVersion& other) const noexcept {
        return major == other.major;
    }
};

struct AgentSnapshot {
    flgod::EntityID id{0};
    uint32_t colony_id{1};
    uint32_t species_id{1};
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> velocity{0.0, 0.0, 0.0};
    double energy{100.0};
    double hunger{0.0};
    std::string action{"Forage"};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"id", id.raw()},
            {"colony_id", colony_id},
            {"species_id", species_id},
            {"position", position},
            {"velocity", velocity},
            {"energy", energy},
            {"hunger", hunger},
            {"action", action}
        };
    }

    static AgentSnapshot from_json(const nlohmann::json& j) {
        AgentSnapshot a;
        a.id = flgod::EntityID(j.value("id", 0ULL));
        a.colony_id = j.value("colony_id", 1U);
        a.species_id = j.value("species_id", 1U);
        if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
            a.position = {j["position"][0].get<double>(), j["position"][1].get<double>(), j["position"][2].get<double>()};
        }
        if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() >= 3) {
            a.velocity = {j["velocity"][0].get<double>(), j["velocity"][1].get<double>(), j["velocity"][2].get<double>()};
        }
        a.energy = j.value("energy", 100.0);
        a.hunger = j.value("hunger", 0.0);
        a.action = j.value("action", "Forage");
        return a;
    }
};

struct ColonySnapshot {
    uint32_t id{1};
    std::array<double, 3> nest{15.0, 3.0, 15.0};
    double radius{30.0};
    double resources{150.0};
    uint32_t pop{10};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"id", id},
            {"nest", nest},
            {"radius", radius},
            {"resources", resources},
            {"pop", pop}
        };
    }

    static ColonySnapshot from_json(const nlohmann::json& j) {
        ColonySnapshot c;
        c.id = j.value("id", 1U);
        if (j.contains("nest") && j["nest"].is_array() && j["nest"].size() >= 3) {
            c.nest = {j["nest"][0].get<double>(), j["nest"][1].get<double>(), j["nest"][2].get<double>()};
        }
        c.radius = j.value("radius", 30.0);
        c.resources = j.value("resources", 150.0);
        c.pop = j.value("pop", 10U);
        return c;
    }
};

struct GodFlySnapshot {
    flgod::EntityID id{1000000000000ULL};
    std::array<double, 3> position{30.0, 8.0, 30.0};
    uint32_t lessons_taught{12};
    std::string active_mode{"Suggestion"};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"id", id.raw()},
            {"position", position},
            {"lessons_taught", lessons_taught},
            {"active_mode", active_mode}
        };
    }

    static GodFlySnapshot from_json(const nlohmann::json& j) {
        GodFlySnapshot gf;
        gf.id = flgod::EntityID(j.value("id", 1000000000000ULL));
        if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
            gf.position = {j["position"][0].get<double>(), j["position"][1].get<double>(), j["position"][2].get<double>()};
        }
        gf.lessons_taught = j.value("lessons_taught", 0U);
        gf.active_mode = j.value("active_mode", "Suggestion");
        return gf;
    }
};

struct EventMessage {
    uint64_t id{1};
    std::string type_name{"LessonTaught"};
    double priority{85.0};
    std::string description{"God Fly Instructed Student on Foraging"};
    uint64_t tick{0};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"id", id},
            {"type_name", type_name},
            {"priority", priority},
            {"description", description},
            {"tick", tick}
        };
    }

    static EventMessage from_json(const nlohmann::json& j) {
        EventMessage ev;
        ev.id = j.value("id", 1ULL);
        ev.type_name = j.value("type_name", "General");
        ev.priority = j.value("priority", 0.0);
        ev.description = j.value("description", "");
        ev.tick = j.value("tick", 0ULL);
        return ev;
    }
};

struct CameraTarget {
    uint32_t channel{0};
    std::string channel_name{"Cam1_GodFly"};
    uint32_t shot{6};
    std::string shot_name{"Orbit"};
    std::array<double, 3> pos{30.0, 11.5, 36.0};
    std::array<double, 3> look_at{30.0, 8.0, 30.0};
    double fov{55.0};
    double distance{7.0};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"channel", channel},
            {"channel_name", channel_name},
            {"shot", shot},
            {"shot_name", shot_name},
            {"current_pose", {
                {"pos", pos},
                {"look_at", look_at},
                {"fov", fov},
                {"distance", distance}
            }}
        };
    }

    static CameraTarget from_json(const nlohmann::json& j) {
        CameraTarget ct;
        ct.channel = j.value("channel", 0U);
        ct.channel_name = j.value("channel_name", "");
        ct.shot = j.value("shot", 0U);
        ct.shot_name = j.value("shot_name", "");
        if (j.contains("current_pose")) {
            const auto& pose = j["current_pose"];
            if (pose.contains("pos") && pose["pos"].is_array() && pose["pos"].size() >= 3) {
                ct.pos = {pose["pos"][0].get<double>(), pose["pos"][1].get<double>(), pose["pos"][2].get<double>()};
            }
            if (pose.contains("look_at") && pose["look_at"].is_array() && pose["look_at"].size() >= 3) {
                ct.look_at = {pose["look_at"][0].get<double>(), pose["look_at"][1].get<double>(), pose["look_at"][2].get<double>()};
            }
            ct.fov = pose.value("fov", 55.0);
            ct.distance = pose.value("distance", 10.0);
        }
        return ct;
    }
};

struct TelemetryMessage {
    double ticks_per_sec{9418.0};
    double soma_rate_mps{128.95};
    std::string gpu_device{"AMD Radeon(TM) Graphics"};
    uint32_t active_agents{20};
    uint32_t alive_agents{20};
    uint32_t total_agents{20};
    uint32_t colony_count{2};

    [[nodiscard]] nlohmann::json to_json() const {
        return nlohmann::json{
            {"ticks_per_sec", ticks_per_sec},
            {"soma_rate_mps", soma_rate_mps},
            {"gpu_device", gpu_device},
            {"active_agents", active_agents},
            {"alive_agents", alive_agents},
            {"total_agents", total_agents},
            {"colony_count", colony_count}
        };
    }

    static TelemetryMessage from_json(const nlohmann::json& j) {
        TelemetryMessage tm;
        tm.ticks_per_sec = j.value("ticks_per_sec", 0.0);
        tm.soma_rate_mps = j.value("soma_rate_mps", 0.0);
        tm.gpu_device = j.value("gpu_device", "");
        tm.active_agents = j.value("active_agents", 0U);
        tm.alive_agents = j.value("alive_agents", 0U);
        tm.total_agents = j.value("total_agents", 0U);
        tm.colony_count = j.value("colony_count", 0U);
        return tm;
    }
};

struct WorldSnapshot {
    ProtocolVersion protocol_version;
    std::string simulation_version{"0.1.0"};
    uint64_t tick{0};
    double elapsed_seconds{0.0};
    std::vector<AgentSnapshot> agents;
    std::vector<ColonySnapshot> colonies;
    GodFlySnapshot god_fly;
    std::vector<EventMessage> events;
    std::vector<CameraTarget> camera_targets;
    TelemetryMessage telemetry;

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["protocol_version"] = protocol_version.to_string();
        j["simulation_version"] = simulation_version;
        j["clock"] = {
            {"tick", tick},
            {"elapsed_seconds", elapsed_seconds},
            {"dt", 0.016666666666666666}
        };

        nlohmann::json j_agents = nlohmann::json::array();
        for (const auto& a : agents) j_agents.push_back(a.to_json());
        j["agents"] = j_agents;

        nlohmann::json j_colonies = nlohmann::json::array();
        for (const auto& c : colonies) j_colonies.push_back(c.to_json());
        j["colonies"] = j_colonies;

        j["god_fly"] = god_fly.to_json();

        nlohmann::json j_events = nlohmann::json::array();
        for (const auto& ev : events) j_events.push_back(ev.to_json());
        j["events"] = j_events;

        nlohmann::json j_cam = nlohmann::json::array();
        for (const auto& ct : camera_targets) j_cam.push_back(ct.to_json());
        j["camera"] = {{"channels", j_cam}};

        j["telemetry"] = telemetry.to_json();

        return j;
    }

    static WorldSnapshot from_json(const nlohmann::json& j) {
        WorldSnapshot ws;
        if (j.contains("protocol_version")) {
            std::string pstr = j["protocol_version"].get<std::string>();
            size_t dot = pstr.find('.');
            if (dot != std::string::npos) {
                ws.protocol_version.major = static_cast<uint32_t>(std::stoul(pstr.substr(0, dot)));
                ws.protocol_version.minor = static_cast<uint32_t>(std::stoul(pstr.substr(dot + 1)));
            }
        }
        ws.simulation_version = j.value("simulation_version", "0.1.0");
        if (j.contains("clock")) {
            ws.tick = j["clock"].value("tick", 0ULL);
            ws.elapsed_seconds = j["clock"].value("elapsed_seconds", 0.0);
        }
        if (j.contains("agents") && j["agents"].is_array()) {
            for (const auto& ja : j["agents"]) {
                ws.agents.push_back(AgentSnapshot::from_json(ja));
            }
        }
        if (j.contains("colonies") && j["colonies"].is_array()) {
            for (const auto& jc : j["colonies"]) {
                ws.colonies.push_back(ColonySnapshot::from_json(jc));
            }
        }
        if (j.contains("god_fly")) {
            ws.god_fly = GodFlySnapshot::from_json(j["god_fly"]);
        }
        if (j.contains("events") && j["events"].is_array()) {
            for (const auto& jev : j["events"]) {
                ws.events.push_back(EventMessage::from_json(jev));
            }
        }
        if (j.contains("camera") && j["camera"].contains("channels") && j["camera"]["channels"].is_array()) {
            for (const auto& jc : j["camera"]["channels"]) {
                ws.camera_targets.push_back(CameraTarget::from_json(jc));
            }
        }
        if (j.contains("telemetry")) {
            ws.telemetry = TelemetryMessage::from_json(j["telemetry"]);
        }
        return ws;
    }
};

} // namespace flgod::integration
