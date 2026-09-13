#pragma once

#include "flgod/world/fields.hpp"
#include "flgod/world/noise.hpp"
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <nlohmann/json.hpp>

namespace flgod {

struct WeatherState {
    double temperature{21.0}; // Celsius
    double humidity{0.45};     // [0.0, 1.0]
    double pressure{1013.25};  // hPa
    Vec3 wind{2.5, 0.0, 1.2};  // m/s
    double precipitation{0.0}; // [0.0, 1.0]
    double visibility{10000.0};// meters

    [[nodiscard]] bool is_raining() const noexcept { return precipitation > 0.05; }
    [[nodiscard]] bool is_storm() const noexcept { return precipitation > 0.6 && wind.length() > 12.0; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_double = [&h](double v) {
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(double));
            h ^= bits;
            h *= 1099511628211ULL;
        };
        mix_double(temperature);
        mix_double(humidity);
        mix_double(pressure);
        mix_double(wind.x);
        mix_double(wind.y);
        mix_double(wind.z);
        mix_double(precipitation);
        mix_double(visibility);
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"temperature", temperature},
            {"humidity", humidity},
            {"pressure", pressure},
            {"wind", {{"x", wind.x}, {"y", wind.y}, {"z", wind.z}}},
            {"precipitation", precipitation},
            {"visibility", visibility}
        };
    }

    void from_json(const nlohmann::json& j) {
        temperature = j.value("temperature", 21.0);
        humidity = j.value("humidity", 0.45);
        pressure = j.value("pressure", 1013.25);
        if (j.contains("wind")) {
            wind.x = j["wind"].value("x", 2.0);
            wind.y = j["wind"].value("y", 0.0);
            wind.z = j["wind"].value("z", 1.0);
        }
        precipitation = j.value("precipitation", 0.0);
        visibility = j.value("visibility", 10000.0);
    }
};

class WeatherSystem {
public:
    explicit WeatherSystem(uint64_t weather_seed = 133702ULL)
        : m_seed(weather_seed),
          m_noise(weather_seed) {}

    void step([[maybe_unused]] double dt, double elapsed_simulation_seconds) {
        // 1. Diurnal cycle (24-hour cycle: day/night)
        // 1 day = 1200 seconds in simulation time (configurable)
        constexpr double DAY_LENGTH_SECONDS = 1200.0;
        double day_fraction = std::fmod(elapsed_simulation_seconds, DAY_LENGTH_SECONDS) / DAY_LENGTH_SECONDS;
        double sun_angle = day_fraction * 2.0 * 3.14159265358979323846;

        // Base diurnal temperature oscillation: peak at midday (fraction ~0.5)
        double diurnal_temp = std::sin(sun_angle - 1.57079632679) * 6.0; // +/- 6C

        // 2. Front noise (slow weather shifts)
        double t_noise = elapsed_simulation_seconds * 0.001;
        double front_shift = m_noise.noise2d(t_noise, 0.5);
        double rain_front = m_noise.noise2d(t_noise * 1.5, 42.0);

        m_state.temperature = 20.0 + diurnal_temp + front_shift * 4.0;
        m_state.humidity = std::clamp(0.5 + front_shift * 0.3 + rain_front * 0.2, 0.1, 0.98);
        m_state.pressure = 1013.25 - front_shift * 18.0;

        // Rain threshold: triggers when humidity > 0.72
        if (m_state.humidity > 0.72) {
            m_state.precipitation = (m_state.humidity - 0.72) / (1.0 - 0.72);
            m_state.visibility = std::max(200.0, 10000.0 * (1.0 - m_state.precipitation * 0.85));
        } else {
            m_state.precipitation = 0.0;
            m_state.visibility = 10000.0;
        }

        // Wind direction and speed shifts
        double wind_x = m_noise.noise2d(t_noise * 2.0, 10.0) * 8.0;
        double wind_z = m_noise.noise2d(t_noise * 2.0, 20.0) * 8.0;
        if (m_state.precipitation > 0.4) {
            wind_x *= 1.8; // Stronger winds in rain
            wind_z *= 1.8;
        }
        m_state.wind = Vec3(wind_x, 0.0, wind_z);
    }

    [[nodiscard]] const WeatherState& state() const noexcept { return m_state; }
    WeatherState& state() noexcept { return m_state; }
    [[nodiscard]] uint64_t seed() const noexcept { return m_seed; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return m_state.compute_hash() ^ (m_seed * 1099511628211ULL);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"seed", m_seed},
            {"state", m_state.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_seed = j.value("seed", 133702ULL);
        m_noise.init(m_seed);
        if (j.contains("state")) {
            m_state.from_json(j["state"]);
        }
    }

private:
    uint64_t m_seed;
    DeterministicNoise m_noise;
    WeatherState m_state{};
};

} // namespace flgod
