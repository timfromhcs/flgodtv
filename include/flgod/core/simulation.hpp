#pragma once

#include "flgod/core/version.hpp"
#include "flgod/core/clock.hpp"
#include "flgod/core/rng.hpp"
#include "flgod/core/entity_id.hpp"
#include "flgod/core/event_bus.hpp"
#include "flgod/core/world_state.hpp"
#include <string>
#include <memory>
#include <fstream>

namespace flgod {

struct SimulationConfig {
    SimulationVersion version{CURRENT_SIMULATION_VERSION};
    RNGSeeds seeds{};
    double fixed_dt{1.0 / 60.0};
    uint64_t start_tick{0};
    double start_time{0.0};
    WorldConfig world_config{};
    std::string experiment_id{"default_experiment"};
};

class Simulation {
public:
    Simulation() = default;
    ~Simulation() {
        if (m_is_initialized) {
            shutdown();
        }
    }

    // Explicit initialization
    void initialize(const SimulationConfig& config) {
        m_config = config;
        WorldStateConfig ws_config{
            .version = config.version,
            .seeds = config.seeds,
            .fixed_dt = config.fixed_dt,
            .start_tick = config.start_tick,
            .start_time = config.start_time,
            .world_config = config.world_config
        };
        m_world_state = WorldState(ws_config);
        m_event_bus.clear_all();
        m_is_initialized = true;
    }

    // Explicit shutdown
    void shutdown() noexcept {
        m_event_bus.clear_all();
        m_is_initialized = false;
    }

    [[nodiscard]] bool is_initialized() const noexcept {
        return m_is_initialized;
    }

    // Single deterministic simulation step
    SimulationStep step() {
        if (!m_is_initialized) {
            throw std::runtime_error("Simulation::step() called on uninitialized simulation!");
        }

        // 1. Advance clock
        SimulationStep s = m_world_state.clock().step();

        // 2. Advance procedural continuous world & weather
        m_world_state.world().step(s.dt, s.elapsed_seconds);

        // 3. Publish TickStart
        TickStartEvent start_ev;
        start_ev.tick = s.tick;
        start_ev.timestamp = s.elapsed_seconds;
        m_event_bus.publish_immediate(start_ev);

        // 4. Process queued events
        m_event_bus.flush(s.tick);

        // 5. Publish TickEnd
        TickEndEvent end_ev;
        end_ev.tick = s.tick;
        end_ev.timestamp = s.elapsed_seconds + s.dt;
        m_event_bus.publish_immediate(end_ev);

        return s;
    }

    void run_ticks(uint64_t count) {
        for (uint64_t i = 0; i < count; ++i) {
            step();
        }
    }

    [[nodiscard]] const SimulationConfig& config() const noexcept { return m_config; }
    [[nodiscard]] const WorldState& state() const noexcept { return m_world_state; }
    [[nodiscard]] WorldState& state() noexcept { return m_world_state; }
    [[nodiscard]] EventBus& event_bus() noexcept { return m_event_bus; }

    [[nodiscard]] uint64_t compute_state_hash() const noexcept {
        return m_world_state.compute_hash();
    }

    [[nodiscard]] nlohmann::json create_checkpoint() const {
        nlohmann::json j;
        j["simulation_version"] = m_config.version.to_string();
        j["experiment_id"] = m_config.experiment_id;
        j["world_state"] = m_world_state.to_json();
        j["checkpoint_hash"] = compute_state_hash();
        return j;
    }

    void restore_checkpoint(const nlohmann::json& checkpoint_json) {
        if (!checkpoint_json.contains("world_state")) {
            throw std::runtime_error("Invalid checkpoint: missing 'world_state'");
        }
        m_world_state.from_json(checkpoint_json["world_state"]);
        m_is_initialized = true;
    }

private:
    SimulationConfig m_config;
    WorldState m_world_state;
    EventBus m_event_bus;
    bool m_is_initialized{false};
};

} // namespace flgod
