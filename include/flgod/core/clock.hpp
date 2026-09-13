#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>

namespace flgod {

struct SimulationStep {
    uint64_t tick{0};
    double dt{1.0 / 60.0};
    double elapsed_seconds{0.0};

    bool operator==(const SimulationStep& other) const = default;
};

class SimulationClock {
public:
    explicit SimulationClock(double fixed_dt = 1.0 / 60.0)
        : m_fixed_dt(fixed_dt) {}

    void reset(uint64_t start_tick = 0, double start_time = 0.0) noexcept {
        m_tick = start_tick;
        m_elapsed_time = start_time;
    }

    SimulationStep step() noexcept {
        SimulationStep s{
            .tick = m_tick,
            .dt = m_fixed_dt,
            .elapsed_seconds = m_elapsed_time
        };
        m_tick += 1;
        m_elapsed_time += m_fixed_dt;
        return s;
    }

    [[nodiscard]] uint64_t tick() const noexcept { return m_tick; }
    [[nodiscard]] double fixed_dt() const noexcept { return m_fixed_dt; }
    [[nodiscard]] double elapsed_time() const noexcept { return m_elapsed_time; }

    void set_tick(uint64_t t) noexcept { m_tick = t; }
    void set_elapsed_time(double t) noexcept { m_elapsed_time = t; }
    void set_fixed_dt(double dt) noexcept { m_fixed_dt = dt; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto hash_byte = [&h](uint8_t b) {
            h ^= b;
            h *= 1099511628211ULL;
        };
        for (int i = 0; i < 8; ++i) {
            hash_byte(static_cast<uint8_t>((m_tick >> (i * 8)) & 0xFF));
        }
        uint64_t dt_bits = 0;
        uint64_t el_bits = 0;
        std::memcpy(&dt_bits, &m_fixed_dt, sizeof(double));
        std::memcpy(&el_bits, &m_elapsed_time, sizeof(double));
        for (int i = 0; i < 8; ++i) {
            hash_byte(static_cast<uint8_t>((dt_bits >> (i * 8)) & 0xFF));
            hash_byte(static_cast<uint8_t>((el_bits >> (i * 8)) & 0xFF));
        }
        return h;
    }

    bool operator==(const SimulationClock& other) const = default;

private:
    uint64_t m_tick{0};
    double m_fixed_dt{1.0 / 60.0};
    double m_elapsed_time{0.0};
};

} // namespace flgod
