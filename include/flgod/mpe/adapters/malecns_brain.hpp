#pragma once

// MaleCNS connectome as a generic MPE brain.
// Fly-specific I/O is translated at THIS boundary; the core only sees
// Observation channels and ActionIntent requests.

#include "flgod/brain/malecns_adapter.hpp"
#include "flgod/world/fields.hpp"
#include "flgod/mpe/interfaces.hpp"
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace flgod::mpe::adapters {

class MaleCNSBrainAdapter : public IBrain {
public:
    explicit MaleCNSBrainAdapter(const std::string& csv_path =
                                     "malecns/data-raw/2023-27-2 soma_sides.csv",
                                 size_t max_somas = 500)
        : m_csv_path(csv_path), m_max_somas(max_somas) {}

    [[nodiscard]] std::string provider() const override { return "malecns"; }

    void initialize(const nlohmann::json& config) override {
        m_csv_path = config.value("csv_path", m_csv_path);
        m_max_somas = config.value("max_somas", m_max_somas);
        m_inner = std::make_unique<brain::MaleCNSAdapter>(m_csv_path, m_max_somas);
        m_steps = 0;
        m_motor = brain::BrainMotorOutput{};
    }

    void observe(const Observation& obs) override { m_last = obs; }

    void step(double dt) override {
        ensure_inner();
        brain::BrainSensoryInput in;
        in.odor_sugar_intensity = clamp01(m_last.get("odor_sugar"));
        in.odor_pheromone_intensity = clamp01(m_last.get("odor_pheromone"));
        in.odor_danger_intensity = clamp01(m_last.get("odor_danger"));
        in.internal_energy = m_last.get("energy", 100.0);
        in.internal_fatigue = m_last.get("fatigue", 0.0);
        in.air_velocity = flgod::Vec3{m_last.get("wind_x"), m_last.get("wind_y"),
                                     m_last.get("wind_z")};
        for (int i = 0; i < 8; ++i) {
            in.left_eye_sectors[static_cast<size_t>(i)] =
                clamp01(m_last.get("eye_l" + std::to_string(i)));
            in.right_eye_sectors[static_cast<size_t>(i)] =
                clamp01(m_last.get("eye_r" + std::to_string(i)));
        }
        brain::BrainMotorOutput out;
        m_motor = brain::BrainMotorOutput{}; // inner step leaves x/z torque stale
        m_inner->step(dt, in, m_motor);
        m_steps++;
    }

    [[nodiscard]] std::vector<ActionIntent> intents() const override {
        std::vector<ActionIntent> out;
        double mag = std::sqrt(m_motor.steering_torque.x * m_motor.steering_torque.x +
                               m_motor.steering_torque.y * m_motor.steering_torque.y +
                               m_motor.steering_torque.z * m_motor.steering_torque.z);
        if (m_motor.proboscis_extension > 0.5) {
            ActionIntent eat;
            eat.action = "Eat";
            eat.priority = 2;
            out.push_back(eat);
        }
        if (mag > 1e-6) {
            ActionIntent mv;
            mv.action = "Move";
            mv.params["dx"] = m_motor.steering_torque.x;
            mv.params["dy"] = m_motor.steering_torque.y;
            mv.params["dz"] = m_motor.steering_torque.z;
            mv.priority = 1;
            out.push_back(mv);
        } else if (out.empty()) {
            ActionIntent rest;
            rest.action = "Rest";
            rest.priority = 0;
            out.push_back(rest);
        }
        return out;
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        ensure_inner();
        return {{"provider", "malecns"},
                {"csv_path", m_csv_path},
                {"max_somas", m_max_somas},
                {"steps", m_steps},
                {"inner", m_inner->to_json()}};
    }

    void from_json(const nlohmann::json& j) override {
        m_csv_path = j.value("csv_path", m_csv_path);
        m_max_somas = j.value("max_somas", m_max_somas);
        // Build topology at the checkpoint's actual soma count: the inner
        // from_json resizes state arrays but does NOT rebuild hemisphere
        // index lists, so construction size must match the recorded state.
        size_t build_n = m_max_somas;
        if (j.contains("inner") && j["inner"].contains("soma_count")) {
            build_n = j["inner"]["soma_count"].get<size_t>();
        }
        m_inner = std::make_unique<brain::MaleCNSAdapter>(m_csv_path, build_n);
        if (j.contains("inner")) {
            m_inner->from_json(j["inner"]);
        }
        if (m_inner->soma_count() != build_n) {
            throw std::runtime_error(
                "MaleCNSBrainAdapter::from_json: connectome data unavailable or "
                "topology mismatch (have " +
                std::to_string(m_inner->soma_count()) + " somas, checkpoint needs " +
                std::to_string(build_n) + ")");
        }
        m_steps = j.value("steps", uint64_t{0});
        m_motor = brain::BrainMotorOutput{};
    }

    [[nodiscard]] uint64_t compute_hash() const override {
        ensure_inner();
        return m_inner->compute_brain_hash() ^ (m_steps * 1099511628211ULL);
    }

    void reset() override {
        m_steps = 0;
        m_last = Observation{};
        m_motor = brain::BrainMotorOutput{};
        m_inner = std::make_unique<brain::MaleCNSAdapter>(m_csv_path, m_max_somas);
    }

    [[nodiscard]] size_t soma_count() const {
        ensure_inner();
        return m_inner->soma_count();
    }

private:
    static double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
    void ensure_inner() const {
        if (!m_inner) {
            m_inner = std::make_unique<brain::MaleCNSAdapter>(m_csv_path, m_max_somas);
        }
    }
    std::string m_csv_path;
    size_t m_max_somas;
    mutable std::unique_ptr<brain::MaleCNSAdapter> m_inner;
    Observation m_last;
    brain::BrainMotorOutput m_motor;
    uint64_t m_steps{0};
};

} // namespace flgod::mpe::adapters
