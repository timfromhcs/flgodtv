#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include <string>
#include <stdexcept>
#include <numbers>
#include <nlohmann/json.hpp>

namespace flgod {

// SplitMix64 generator - used for robust initialization and state mixing
inline uint64_t splitmix64(uint64_t& state) noexcept {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Xoshiro256++ generator - fast, high quality 256-bit PRNG
class RNGStream;
using Xoshiro256PlusPlus = RNGStream;

class RNGStream {
public:
    explicit RNGStream(uint64_t seed = 0x853C49E6748FEA9BULL) {
        reseed(seed);
    }

    void reseed(uint64_t seed) noexcept {
        uint64_t sm_state = seed;
        m_s[0] = splitmix64(sm_state);
        m_s[1] = splitmix64(sm_state);
        m_s[2] = splitmix64(sm_state);
        m_s[3] = splitmix64(sm_state);
        if (m_s[0] == 0 && m_s[1] == 0 && m_s[2] == 0 && m_s[3] == 0) {
            m_s[0] = 1;
        }
        m_has_cached_normal = false;
    }

    [[nodiscard]] std::array<uint64_t, 4> get_state() const noexcept {
        return m_s;
    }

    void set_state(const std::array<uint64_t, 4>& state) noexcept {
        m_s = state;
        m_has_cached_normal = false;
    }

    uint64_t next_u64() noexcept {
        const uint64_t result = rotl(m_s[0] + m_s[3], 23) + m_s[0];
        const uint64_t t = m_s[1] << 17;

        m_s[2] ^= m_s[0];
        m_s[3] ^= m_s[1];
        m_s[1] ^= m_s[2];
        m_s[0] ^= m_s[3];

        m_s[2] ^= t;
        m_s[3] = rotl(m_s[3], 45);

        return result;
    }

    uint32_t next_u32() noexcept {
        return static_cast<uint32_t>(next_u64() >> 32);
    }

    // Uniform double in [0.0, 1.0)
    double next_double() noexcept {
        return (next_u64() >> 11) * (1.0 / (1ULL << 53));
    }

    // Uniform float in [0.0f, 1.0f)
    float next_float() noexcept {
        return (next_u32() >> 8) * (1.0f / (1U << 24));
    }

    // Uniform integer in [min, max] inclusive
    int64_t uniform_int(int64_t min, int64_t max) noexcept {
        if (min >= max) return min;
        uint64_t range = static_cast<uint64_t>(max - min + 1);
        uint64_t x = next_u64();
        return min + static_cast<int64_t>(x % range);
    }

    // Uniform real in [min, max)
    double uniform_real(double min, double max) noexcept {
        return min + (max - min) * next_double();
    }

    // Gaussian / Normal distribution via deterministic Box-Muller
    double gaussian(double mean = 0.0, double stddev = 1.0) noexcept {
        if (m_has_cached_normal) {
            m_has_cached_normal = false;
            return mean + m_cached_normal * stddev;
        }

        double u1 = next_double();
        while (u1 <= 1e-15) { // Prevent log(0)
            u1 = next_double();
        }
        double u2 = next_double();

        double mag = std::sqrt(-2.0 * std::log(u1));
        double z0 = mag * std::cos(2.0 * 3.14159265358979323846 * u2);
        double z1 = mag * std::sin(2.0 * 3.14159265358979323846 * u2);

        m_cached_normal = z1;
        m_has_cached_normal = true;

        return mean + z0 * stddev;
    }

    double next_gaussian(double mean = 0.0, double stddev = 1.0) noexcept {
        return gaussian(mean, stddev);
    }

    bool coin_flip(double probability = 0.5) noexcept {
        return next_double() < probability;
    }

    // Fork a child deterministic stream with decorrelated seed
    [[nodiscard]] RNGStream fork(uint64_t branch_tag) noexcept {
        uint64_t child_seed = next_u64() ^ (branch_tag + 0x9E3779B97F4A7C15ULL);
        return RNGStream(child_seed);
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        for (uint64_t v : m_s) {
            for (int i = 0; i < 8; ++i) {
                h ^= static_cast<uint8_t>((v >> (i * 8)) & 0xFF);
                h *= 1099511628211ULL;
            }
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"s0", m_s[0]}, {"s1", m_s[1]}, {"s2", m_s[2]}, {"s3", m_s[3]},
            {"cached_normal", m_cached_normal},
            {"has_cached_normal", m_has_cached_normal}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("s0")) m_s[0] = j["s0"].get<uint64_t>();
        if (j.contains("s1")) m_s[1] = j["s1"].get<uint64_t>();
        if (j.contains("s2")) m_s[2] = j["s2"].get<uint64_t>();
        if (j.contains("s3")) m_s[3] = j["s3"].get<uint64_t>();
        m_cached_normal = j.value("cached_normal", 0.0);
        m_has_cached_normal = j.value("has_cached_normal", false);
    }

    bool operator==(const RNGStream& other) const noexcept {
        return m_s == other.m_s;
    }

private:
    static constexpr uint64_t rotl(const uint64_t x, int k) noexcept {
        return (x << k) | (x >> (64 - k));
    }

    std::array<uint64_t, 4> m_s{0, 0, 0, 0};
    double m_cached_normal{0.0};
    bool m_has_cached_normal{false};
};

struct RNGSeeds {
    uint64_t world_seed{133701ULL};
    uint64_t weather_seed{133702ULL};
    uint64_t physics_seed{133703ULL};
    uint64_t agent_seed{133704ULL};
    uint64_t genome_seed{133705ULL};
    uint64_t event_seed{133706ULL};
    uint64_t learning_seed{133707ULL};
    uint64_t render_seed{133708ULL};

    bool operator==(const RNGSeeds& other) const = default;
};

class DeterministicRNG {
public:
    explicit DeterministicRNG(const RNGSeeds& seeds = RNGSeeds{})
        : m_seeds(seeds),
          m_world(seeds.world_seed),
          m_weather(seeds.weather_seed),
          m_physics(seeds.physics_seed),
          m_agent(seeds.agent_seed),
          m_genome(seeds.genome_seed),
          m_event(seeds.event_seed),
          m_learning(seeds.learning_seed),
          m_render(seeds.render_seed) {}

    void reseed_all(const RNGSeeds& seeds) noexcept {
        m_seeds = seeds;
        m_world.reseed(seeds.world_seed);
        m_weather.reseed(seeds.weather_seed);
        m_physics.reseed(seeds.physics_seed);
        m_agent.reseed(seeds.agent_seed);
        m_genome.reseed(seeds.genome_seed);
        m_event.reseed(seeds.event_seed);
        m_learning.reseed(seeds.learning_seed);
        m_render.reseed(seeds.render_seed);
    }

    [[nodiscard]] const RNGSeeds& seeds() const noexcept { return m_seeds; }

    RNGStream& world() noexcept { return m_world; }
    RNGStream& weather() noexcept { return m_weather; }
    RNGStream& physics() noexcept { return m_physics; }
    RNGStream& agent() noexcept { return m_agent; }
    RNGStream& genome() noexcept { return m_genome; }
    RNGStream& event() noexcept { return m_event; }
    RNGStream& learning() noexcept { return m_learning; }
    RNGStream& render() noexcept { return m_render; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto combine = [&h](uint64_t val) {
            h ^= val;
            h *= 1099511628211ULL;
        };
        combine(m_world.compute_hash());
        combine(m_weather.compute_hash());
        combine(m_physics.compute_hash());
        combine(m_agent.compute_hash());
        combine(m_genome.compute_hash());
        combine(m_event.compute_hash());
        combine(m_learning.compute_hash());
        combine(m_render.compute_hash());
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"seeds", {
                {"world", m_seeds.world_seed},
                {"weather", m_seeds.weather_seed},
                {"physics", m_seeds.physics_seed},
                {"agent", m_seeds.agent_seed},
                {"genome", m_seeds.genome_seed},
                {"event", m_seeds.event_seed},
                {"learning", m_seeds.learning_seed},
                {"render", m_seeds.render_seed}
            }},
            {"streams", {
                {"world", m_world.to_json()},
                {"weather", m_weather.to_json()},
                {"physics", m_physics.to_json()},
                {"agent", m_agent.to_json()},
                {"genome", m_genome.to_json()},
                {"event", m_event.to_json()},
                {"learning", m_learning.to_json()},
                {"render", m_render.to_json()}
            }}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("seeds")) {
            const auto& s = j["seeds"];
            m_seeds.world_seed = s.value("world", 133701ULL);
            m_seeds.weather_seed = s.value("weather", 133702ULL);
            m_seeds.physics_seed = s.value("physics", 133703ULL);
            m_seeds.agent_seed = s.value("agent", 133704ULL);
            m_seeds.genome_seed = s.value("genome", 133705ULL);
            m_seeds.event_seed = s.value("event", 133706ULL);
            m_seeds.learning_seed = s.value("learning", 133707ULL);
            m_seeds.render_seed = s.value("render", 133708ULL);
        }
        if (j.contains("streams")) {
            const auto& st = j["streams"];
            if (st.contains("world")) m_world.from_json(st["world"]);
            if (st.contains("weather")) m_weather.from_json(st["weather"]);
            if (st.contains("physics")) m_physics.from_json(st["physics"]);
            if (st.contains("agent")) m_agent.from_json(st["agent"]);
            if (st.contains("genome")) m_genome.from_json(st["genome"]);
            if (st.contains("event")) m_event.from_json(st["event"]);
            if (st.contains("learning")) m_learning.from_json(st["learning"]);
            if (st.contains("render")) m_render.from_json(st["render"]);
        }
    }

    bool operator==(const DeterministicRNG& other) const noexcept {
        return m_world == other.m_world &&
               m_weather == other.m_weather &&
               m_physics == other.m_physics &&
               m_agent == other.m_agent &&
               m_genome == other.m_genome &&
               m_event == other.m_event &&
               m_learning == other.m_learning &&
               m_render == other.m_render;
    }

private:
    RNGSeeds m_seeds;
    RNGStream m_world;
    RNGStream m_weather;
    RNGStream m_physics;
    RNGStream m_agent;
    RNGStream m_genome;
    RNGStream m_event;
    RNGStream m_learning;
    RNGStream m_render;
};

} // namespace flgod
