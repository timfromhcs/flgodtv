#pragma once

#include "flgod/core/version.hpp"
#include "flgod/core/clock.hpp"
#include "flgod/core/rng.hpp"
#include "flgod/core/entity_id.hpp"
#include "flgod/world/world.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace flgod {

struct WorldStateConfig {
    SimulationVersion version{CURRENT_SIMULATION_VERSION};
    RNGSeeds seeds{};
    double fixed_dt{1.0 / 60.0};
    uint64_t start_tick{0};
    double start_time{0.0};
    WorldConfig world_config{};
};

class WorldState {
public:
    WorldState() = default;
    explicit WorldState(const WorldStateConfig& config)
        : m_version(config.version),
          m_clock(config.fixed_dt),
          m_rng(config.seeds),
          m_world(config.world_config) {
        m_clock.reset(config.start_tick, config.start_time);
    }

    [[nodiscard]] const SimulationVersion& version() const noexcept { return m_version; }
    [[nodiscard]] const SimulationClock& clock() const noexcept { return m_clock; }
    [[nodiscard]] SimulationClock& clock() noexcept { return m_clock; }
    [[nodiscard]] const DeterministicRNG& rng() const noexcept { return m_rng; }
    [[nodiscard]] DeterministicRNG& rng() noexcept { return m_rng; }
    [[nodiscard]] const EntityIDAllocator& id_allocator() const noexcept { return m_id_allocator; }
    [[nodiscard]] EntityIDAllocator& id_allocator() noexcept { return m_id_allocator; }
    [[nodiscard]] const World& world() const noexcept { return m_world; }
    [[nodiscard]] World& world() noexcept { return m_world; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto combine = [&h](uint64_t val) {
            h ^= val;
            h *= 1099511628211ULL;
        };
        combine(m_version.compute_hash());
        combine(m_clock.compute_hash());
        combine(m_rng.compute_hash());
        combine(m_id_allocator.compute_hash());
        combine(m_world.compute_world_hash());
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["version"] = {
            {"major", m_version.major},
            {"minor", m_version.minor},
            {"patch", m_version.patch},
            {"schema", m_version.schema_version},
            {"build_meta", m_version.build_meta}
        };
        j["clock"] = {
            {"tick", m_clock.tick()},
            {"dt", m_clock.fixed_dt()},
            {"elapsed_time", m_clock.elapsed_time()}
        };
        j["rng"] = m_rng.to_json();
        j["id_allocator"] = {
            {"next_index", m_id_allocator.next_index()},
            {"current_generation", m_id_allocator.current_generation()}
        };
        j["world"] = m_world.to_json();
        j["state_hash"] = compute_hash();
        return j;
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("version")) {
            m_version.major = j["version"].value("major", 0u);
            m_version.minor = j["version"].value("minor", 1u);
            m_version.patch = j["version"].value("patch", 0u);
            m_version.schema_version = j["version"].value("schema", 1u);
            m_version.build_meta = j["version"].value("build_meta", std::string("dev"));
        }
        if (j.contains("clock")) {
            m_clock.set_fixed_dt(j["clock"].value("dt", 1.0 / 60.0));
            m_clock.set_tick(j["clock"].value("tick", 0ULL));
            m_clock.set_elapsed_time(j["clock"].value("elapsed_time", 0.0));
        }
        if (j.contains("rng")) {
            m_rng.from_json(j["rng"]);
        } else if (j.contains("rng_seeds")) {
            RNGSeeds s;
            s.world_seed = j["rng_seeds"].value("world", 133701ULL);
            s.weather_seed = j["rng_seeds"].value("weather", 133702ULL);
            s.physics_seed = j["rng_seeds"].value("physics", 133703ULL);
            s.agent_seed = j["rng_seeds"].value("agent", 133704ULL);
            s.genome_seed = j["rng_seeds"].value("genome", 133705ULL);
            s.event_seed = j["rng_seeds"].value("event", 133706ULL);
            s.learning_seed = j["rng_seeds"].value("learning", 133707ULL);
            s.render_seed = j["rng_seeds"].value("render", 133708ULL);
            m_rng.reseed_all(s);
        }
        if (j.contains("id_allocator")) {
            uint64_t next_idx = j["id_allocator"].value("next_index", 1ULL);
            uint16_t gen = j["id_allocator"].value("current_generation", static_cast<uint16_t>(1));
            m_id_allocator.reset(next_idx, gen);
        }
        if (j.contains("world")) {
            m_world.from_json(j["world"]);
        }
    }

    bool operator==(const WorldState& other) const noexcept {
        return m_version == other.m_version &&
               m_clock == other.m_clock &&
               m_rng == other.m_rng &&
               m_id_allocator.next_index() == other.m_id_allocator.next_index() &&
               m_id_allocator.current_generation() == other.m_id_allocator.current_generation();
    }

private:
    SimulationVersion m_version{CURRENT_SIMULATION_VERSION};
    SimulationClock m_clock;
    DeterministicRNG m_rng;
    EntityIDAllocator m_id_allocator;
    World m_world;
};

} // namespace flgod
