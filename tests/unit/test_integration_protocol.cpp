#include <cassert>
#include <iostream>
#include "flgod/integration/protocol.hpp"

using namespace flgod;
using namespace flgod::integration;

int main() {
    std::cout << "[IntegrationProtocolTest] Starting unit tests for Integration Protocol..." << std::endl;

    // 1. Protocol Version Compatibility
    ProtocolVersion v1{1, 0};
    ProtocolVersion v1_1{1, 1};
    ProtocolVersion v2{2, 0};
    assert(v1.is_compatible(v1_1));
    assert(!v1.is_compatible(v2));
    assert(v1.to_string() == "1.0");

    // 2. Agent Snapshot Roundtrip
    AgentSnapshot agent;
    agent.id = EntityID(42);
    agent.colony_id = 1;
    agent.species_id = 2;
    agent.position = {12.5, 3.2, 14.8};
    agent.velocity = {0.5, 0.0, -0.2};
    agent.energy = 88.5;
    agent.hunger = 12.0;
    agent.action = "Forage";

    nlohmann::json a_json = agent.to_json();
    assert(a_json["id"] == 42);
    AgentSnapshot a_parsed = AgentSnapshot::from_json(a_json);
    assert(a_parsed.id.raw() == 42);
    assert(a_parsed.species_id == 2);
    assert(a_parsed.position[0] == 12.5);
    assert(a_parsed.action == "Forage");

    // 3. Colony & God Fly Roundtrip
    ColonySnapshot colony;
    colony.id = 1;
    colony.nest = {15.0, 3.0, 15.0};
    colony.radius = 30.0;
    colony.resources = 150.0;
    colony.pop = 10;

    nlohmann::json c_json = colony.to_json();
    ColonySnapshot c_parsed = ColonySnapshot::from_json(c_json);
    assert(c_parsed.id == 1);
    assert(c_parsed.nest[0] == 15.0);
    assert(c_parsed.resources == 150.0);

    GodFlySnapshot gf;
    gf.id = EntityID(1000000000000ULL);
    gf.position = {30.0, 8.0, 30.0};
    gf.lessons_taught = 15;
    gf.active_mode = "Suggestion";

    nlohmann::json gf_json = gf.to_json();
    GodFlySnapshot gf_parsed = GodFlySnapshot::from_json(gf_json);
    assert(gf_parsed.id.raw() == 1000000000000ULL);
    assert(gf_parsed.lessons_taught == 15);

    // 4. WorldSnapshot Composite Protocol Roundtrip
    WorldSnapshot ws;
    ws.protocol_version = {1, 0};
    ws.simulation_version = "0.1.0";
    ws.tick = 120;
    ws.elapsed_seconds = 2.0;
    ws.agents.push_back(agent);
    ws.colonies.push_back(colony);
    ws.god_fly = gf;

    EventMessage ev;
    ev.id = 1;
    ev.type_name = "Speciation";
    ev.priority = 95.0;
    ev.description = "Lineage Divergence";
    ev.tick = 120;
    ws.events.push_back(ev);

    CameraTarget ct;
    ct.channel = 0;
    ct.channel_name = "Cam1_GodFly";
    ct.shot = 6;
    ct.shot_name = "Orbit";
    ct.pos = {30.0, 11.5, 36.0};
    ct.look_at = {30.0, 8.0, 30.0};
    ct.fov = 55.0;
    ct.distance = 7.0;
    ws.camera_targets.push_back(ct);

    ws.telemetry.ticks_per_sec = 9418.0;
    ws.telemetry.soma_rate_mps = 128.95;
    ws.telemetry.alive_agents = 20;

    nlohmann::json ws_json = ws.to_json();
    assert(ws_json["protocol_version"] == "1.0");
    assert(ws_json["clock"]["tick"] == 120);

    WorldSnapshot ws_parsed = WorldSnapshot::from_json(ws_json);
    assert(ws_parsed.protocol_version.major == 1);
    assert(ws_parsed.protocol_version.minor == 0);
    assert(ws_parsed.tick == 120);
    assert(ws_parsed.agents.size() == 1);
    assert(ws_parsed.agents[0].id.raw() == 42);
    assert(ws_parsed.colonies.size() == 1);
    assert(ws_parsed.god_fly.lessons_taught == 15);
    assert(ws_parsed.events.size() == 1);
    assert(ws_parsed.events[0].type_name == "Speciation");
    assert(ws_parsed.camera_targets.size() == 1);
    assert(ws_parsed.camera_targets[0].channel_name == "Cam1_GodFly");
    assert(ws_parsed.telemetry.alive_agents == 20);

    std::cout << "[IntegrationProtocolTest] ALL INTEGRATION PROTOCOL TESTS PASSED (100%)!" << std::endl;
    return 0;
}
