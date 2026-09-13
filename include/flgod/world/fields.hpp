#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>
#include <nlohmann/json.hpp>

namespace flgod {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vec3() noexcept = default;
    constexpr Vec3(double px, double py, double pz) noexcept : x(px), y(py), z(pz) {}

    constexpr Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(double s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr bool operator==(const Vec3& o) const noexcept = default;

    [[nodiscard]] double length() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }
};

// Generic 2D Scalar Field with spatial grid and continuous bilinear interpolation
class ScalarField2D {
public:
    ScalarField2D() = default;
    ScalarField2D(uint32_t width, uint32_t height, double cell_size = 1.0, double origin_x = 0.0, double origin_y = 0.0, double init_val = 0.0)
        : m_width(width), m_height(height), m_cell_size(cell_size),
          m_origin_x(origin_x), m_origin_y(origin_y),
          m_data(width * height, init_val) {}

    [[nodiscard]] uint32_t width() const noexcept { return m_width; }
    [[nodiscard]] uint32_t height() const noexcept { return m_height; }
    [[nodiscard]] double cell_size() const noexcept { return m_cell_size; }
    [[nodiscard]] double origin_x() const noexcept { return m_origin_x; }
    [[nodiscard]] double origin_y() const noexcept { return m_origin_y; }
    [[nodiscard]] const std::vector<double>& data() const noexcept { return m_data; }
    std::vector<double>& data() noexcept { return m_data; }

    [[nodiscard]] double get_grid(uint32_t gx, uint32_t gy) const noexcept {
        if (gx >= m_width || gy >= m_height) return 0.0;
        return m_data[gy * m_width + gx];
    }

    void set_grid(uint32_t gx, uint32_t gy, double val) noexcept {
        if (gx < m_width && gy < m_height) {
            m_data[gy * m_width + gx] = val;
        }
    }

    // Continuous bilinear sampling at world coordinates (wx, wy)
    [[nodiscard]] double sample(double wx, double wy) const noexcept {
        if (m_width == 0 || m_height == 0) return 0.0;

        double fx = (wx - m_origin_x) / m_cell_size;
        double fy = (wy - m_origin_y) / m_cell_size;

        fx = std::clamp(fx, 0.0, static_cast<double>(m_width - 1));
        fy = std::clamp(fy, 0.0, static_cast<double>(m_height - 1));

        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));
        int x1 = std::min(x0 + 1, static_cast<int>(m_width - 1));
        int y1 = std::min(y0 + 1, static_cast<int>(m_height - 1));

        double tx = fx - x0;
        double ty = fy - y0;

        double c00 = get_grid(x0, y0);
        double c10 = get_grid(x1, y0);
        double c01 = get_grid(x0, y1);
        double c11 = get_grid(x1, y1);

        double top = c00 + tx * (c10 - c00);
        double bottom = c01 + tx * (c11 - c01);
        return top + ty * (bottom - top);
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        for (double val : m_data) {
            uint64_t bits = 0;
            std::memcpy(&bits, &val, sizeof(double));
            for (int i = 0; i < 8; ++i) {
                h ^= static_cast<uint8_t>((bits >> (i * 8)) & 0xFF);
                h *= 1099511628211ULL;
            }
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"width", m_width},
            {"height", m_height},
            {"cell_size", m_cell_size},
            {"origin_x", m_origin_x},
            {"origin_y", m_origin_y},
            {"data", m_data}
        };
    }

    void from_json(const nlohmann::json& j) {
        m_width = j.value("width", 0u);
        m_height = j.value("height", 0u);
        m_cell_size = j.value("cell_size", 1.0);
        m_origin_x = j.value("origin_x", 0.0);
        m_origin_y = j.value("origin_y", 0.0);
        if (j.contains("data")) {
            m_data = j["data"].get<std::vector<double>>();
        }
    }

private:
    uint32_t m_width{0};
    uint32_t m_height{0};
    double m_cell_size{1.0};
    double m_origin_x{0.0};
    double m_origin_y{0.0};
    std::vector<double> m_data;
};

// 1. WindField: 3D vector flow field
class WindField {
public:
    WindField() = default;
    WindField(uint32_t w, uint32_t h, double cell_size)
        : m_vx(w, h, cell_size), m_vy(w, h, cell_size), m_vz(w, h, cell_size) {}

    [[nodiscard]] Vec3 sample(double wx, double wy) const noexcept {
        return {m_vx.sample(wx, wy), m_vy.sample(wx, wy), m_vz.sample(wx, wy)};
    }

    void set(uint32_t gx, uint32_t gy, const Vec3& vel) noexcept {
        m_vx.set_grid(gx, gy, vel.x);
        m_vy.set_grid(gx, gy, vel.y);
        m_vz.set_grid(gx, gy, vel.z);
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return m_vx.compute_hash() ^ (m_vy.compute_hash() << 1) ^ (m_vz.compute_hash() << 2);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"vx", m_vx.to_json()}, {"vy", m_vy.to_json()}, {"vz", m_vz.to_json()}};
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("vx")) m_vx.from_json(j["vx"]);
        if (j.contains("vy")) m_vy.from_json(j["vy"]);
        if (j.contains("vz")) m_vz.from_json(j["vz"]);
    }

private:
    ScalarField2D m_vx;
    ScalarField2D m_vy;
    ScalarField2D m_vz;
};

// 2. TemperatureField (Celsius)
using TemperatureField = ScalarField2D;

// 3. HumidityField ([0.0, 1.0])
using HumidityField = ScalarField2D;

// 4. MoistureField ([0.0, 1.0] soil/ground moisture)
using MoistureField = ScalarField2D;

// 5. WaterField (surface water height / volume)
class WaterField {
public:
    WaterField() = default;
    WaterField(uint32_t w, uint32_t h, double cell_size)
        : m_surface_height(w, h, cell_size),
          m_depth(w, h, cell_size),
          m_flow_x(w, h, cell_size),
          m_flow_y(w, h, cell_size) {}

    ScalarField2D& surface_height() noexcept { return m_surface_height; }
    [[nodiscard]] const ScalarField2D& surface_height() const noexcept { return m_surface_height; }

    ScalarField2D& depth() noexcept { return m_depth; }
    [[nodiscard]] const ScalarField2D& depth() const noexcept { return m_depth; }

    [[nodiscard]] Vec3 flow_at(double wx, double wy) const noexcept {
        return {m_flow_x.sample(wx, wy), m_flow_y.sample(wx, wy), 0.0};
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return m_surface_height.compute_hash() ^ (m_depth.compute_hash() << 1);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"surface_height", m_surface_height.to_json()},
            {"depth", m_depth.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("surface_height")) m_surface_height.from_json(j["surface_height"]);
        if (j.contains("depth")) m_depth.from_json(j["depth"]);
    }

private:
    ScalarField2D m_surface_height;
    ScalarField2D m_depth;
    ScalarField2D m_flow_x;
    ScalarField2D m_flow_y;
};

// 6. FireField: fuel, temperature, combustion progress
class FireField {
public:
    FireField() = default;
    FireField(uint32_t w, uint32_t h, double cell_size)
        : m_fuel(w, h, cell_size, 0.0, 0.0, 1.0),
          m_intensity(w, h, cell_size, 0.0, 0.0, 0.0) {}

    ScalarField2D& fuel() noexcept { return m_fuel; }
    [[nodiscard]] const ScalarField2D& fuel() const noexcept { return m_fuel; }

    ScalarField2D& intensity() noexcept { return m_intensity; }
    [[nodiscard]] const ScalarField2D& intensity() const noexcept { return m_intensity; }

    void step_fire(double dt, const WindField& wind, const MoistureField& moisture) {
        // Fire propagation step based on fuel, moisture, and wind
        uint32_t w = m_fuel.width();
        uint32_t h = m_fuel.height();
        ScalarField2D next_intensity = m_intensity;

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                double cur_int = m_intensity.get_grid(x, y);
                double fuel = m_fuel.get_grid(x, y);
                double moist = moisture.get_grid(x, y);

                if (cur_int > 0.05 && fuel > 0.01) {
                    // Burn fuel
                    double burn_rate = 0.15 * cur_int * (1.0 - 0.5 * moist) * dt;
                    double new_fuel = std::max(0.0, fuel - burn_rate);
                    m_fuel.set_grid(x, y, new_fuel);

                    // If fuel exhausted, extinguish
                    if (new_fuel <= 0.001) {
                        next_intensity.set_grid(x, y, 0.0);
                    } else {
                        // Spread to neighbors influenced by wind
                        Vec3 w_vec = wind.sample(x * m_fuel.cell_size(), y * m_fuel.cell_size());
                        int dx[4] = {-1, 1, 0, 0};
                        int dy[4] = {0, 0, -1, 1};
                        for (int k = 0; k < 4; ++k) {
                            int nx = static_cast<int>(x) + dx[k];
                            int ny = static_cast<int>(y) + dy[k];
                            if (nx >= 0 && nx < static_cast<int>(w) && ny >= 0 && ny < static_cast<int>(h)) {
                                double n_fuel = m_fuel.get_grid(nx, ny);
                                double n_moist = moisture.get_grid(nx, ny);
                                double ignition_threshold = 0.3 + 0.4 * n_moist;
                                if (n_fuel > 0.1 && cur_int > ignition_threshold) {
                                    double spread = 0.05 * cur_int * dt;
                                    double old_int = next_intensity.get_grid(nx, ny);
                                    next_intensity.set_grid(nx, ny, std::min(1.0, old_int + spread));
                                }
                            }
                        }
                    }
                }
            }
        }
        m_intensity = next_intensity;
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return m_fuel.compute_hash() ^ (m_intensity.compute_hash() << 1);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"fuel", m_fuel.to_json()}, {"intensity", m_intensity.to_json()}};
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("fuel")) m_fuel.from_json(j["fuel"]);
        if (j.contains("intensity")) m_intensity.from_json(j["intensity"]);
    }

private:
    ScalarField2D m_fuel;
    ScalarField2D m_intensity;
};

// 7. EcologyField: vegetation biomass, nectar / food density, organic decay
class EcologyField {
public:
    EcologyField() = default;
    EcologyField(uint32_t w, uint32_t h, double cell_size)
        : m_biomass(w, h, cell_size, 0.0, 0.0, 0.5),
          m_nectar(w, h, cell_size, 0.0, 0.0, 0.2),
          m_decay(w, h, cell_size, 0.0, 0.0, 0.0) {}

    ScalarField2D& biomass() noexcept { return m_biomass; }
    [[nodiscard]] const ScalarField2D& biomass() const noexcept { return m_biomass; }

    ScalarField2D& nectar() noexcept { return m_nectar; }
    [[nodiscard]] const ScalarField2D& nectar() const noexcept { return m_nectar; }

    ScalarField2D& decay() noexcept { return m_decay; }
    [[nodiscard]] const ScalarField2D& decay() const noexcept { return m_decay; }

    void step_ecology(double dt, const TemperatureField& temp, const MoistureField& moist) {
        uint32_t w = m_biomass.width();
        uint32_t h = m_biomass.height();

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                double t = temp.get_grid(x, y);
                double m = moist.get_grid(x, y);
                double b = m_biomass.get_grid(x, y);

                // Growth favorable when temp is 15-30C and moisture > 0.3
                double temp_factor = std::clamp((t - 5.0) / 20.0, 0.0, 1.0) * std::clamp((40.0 - t) / 15.0, 0.0, 1.0);
                double growth_rate = 0.02 * temp_factor * m * (1.0 - b);
                double new_b = std::clamp(b + growth_rate * dt, 0.0, 1.0);
                m_biomass.set_grid(x, y, new_b);

                // Nectar production proportional to healthy biomass
                double nectar_prod = 0.05 * new_b * temp_factor * dt;
                double cur_nectar = m_nectar.get_grid(x, y);
                m_nectar.set_grid(x, y, std::clamp(cur_nectar + nectar_prod - 0.01 * cur_nectar * dt, 0.0, 1.0));
            }
        }
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return m_biomass.compute_hash() ^ (m_nectar.compute_hash() << 1) ^ (m_decay.compute_hash() << 2);
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"biomass", m_biomass.to_json()},
            {"nectar", m_nectar.to_json()},
            {"decay", m_decay.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("biomass")) m_biomass.from_json(j["biomass"]);
        if (j.contains("nectar")) m_nectar.from_json(j["nectar"]);
        if (j.contains("decay")) m_decay.from_json(j["decay"]);
    }

private:
    ScalarField2D m_biomass;
    ScalarField2D m_nectar;
    ScalarField2D m_decay;
};

} // namespace flgod
