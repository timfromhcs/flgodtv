#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <filesystem>
#include "flgod/brain/fly_brain_interface.hpp"

namespace flgod::brain {

class MaleCNSAdapter : public IFlyBrain {
private:
    std::vector<NeuronSoma> m_somas;
    std::vector<double> m_potentials; // Membrane potential per neuron
    std::vector<double> m_activations;// Firing rate [0.0, 1.0] per neuron
    
    // Population indices
    std::vector<size_t> m_left_somas;
    std::vector<size_t> m_right_somas;

    uint64_t m_step_count{0};
    bool m_is_initialized{false};

public:
    explicit MaleCNSAdapter(const std::string& csv_path = "malecns/data-raw/2023-27-2 soma_sides.csv", size_t max_somas = 10000) {
        if (!csv_path.empty()) {
            load_from_csv(csv_path, max_somas);
        }
        if (m_somas.empty()) {
            generate_baseline_topology(1000);
        }
        init_state();
    }

    bool load_from_csv(const std::string& filepath, size_t max_somas) {
        std::string actual_path = filepath;
        if (!std::filesystem::exists(actual_path)) {
            if (std::filesystem::exists("../" + filepath)) {
                actual_path = "../" + filepath;
            } else if (std::filesystem::exists("../../" + filepath)) {
                actual_path = "../../" + filepath;
            }
        }
        std::ifstream file(actual_path);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        // Read header
        if (!std::getline(file, line)) return false;

        m_somas.clear();
        m_somas.reserve(std::min<size_t>(max_somas, 150000));

        // Columns: ,nucleus_id,nucleus_size,nx,ny,nz,body,body_size,tbars,tail_x,tail_y,tail_z,tail_distance,soma_side
        while (std::getline(file, line) && m_somas.size() < max_somas) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> row;
            while (std::getline(ss, token, ',')) {
                row.push_back(token);
            }

            if (row.size() < 14) continue;

            NeuronSoma soma;
            try {
                soma.nucleus_id = std::stoull(row[1]);
                // nx, ny, nz are in nanometers; convert to micrometers
                soma.x_um = std::stod(row[3]) / 1000.0;
                soma.y_um = std::stod(row[4]) / 1000.0;
                soma.z_um = std::stod(row[5]) / 1000.0;
                soma.body_id = std::stoull(row[6]);
                soma.body_size = static_cast<uint32_t>(std::stoul(row[7]));
                soma.is_left_hemisphere = (row[13] == "L");
                m_somas.push_back(soma);
            } catch (...) {
                continue;
            }
        }

        file.close();
        if (!m_somas.empty()) {
            init_state();
            return true;
        }
        return false;
    }

    void generate_baseline_topology(size_t count) {
        m_somas.clear();
        m_somas.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            NeuronSoma s;
            s.body_id = 1000000ULL + i;
            s.nucleus_id = 2000000ULL + i;
            s.is_left_hemisphere = (i % 2 == 0);
            s.x_um = (s.is_left_hemisphere ? -40.0 : 40.0) + (i % 20);
            s.y_um = 20.0 + (i % 30);
            s.z_um = 10.0 + (i % 15);
            s.body_size = 500 + static_cast<uint32_t>(i % 500);
            m_somas.push_back(s);
        }
    }

    void init_state() {
        m_potentials.assign(m_somas.size(), -65.0); // Resting potential: -65 mV
        m_activations.assign(m_somas.size(), 0.0);
        m_left_somas.clear();
        m_right_somas.clear();

        for (size_t i = 0; i < m_somas.size(); ++i) {
            if (m_somas[i].is_left_hemisphere) {
                m_left_somas.push_back(i);
            } else {
                m_right_somas.push_back(i);
            }
        }
        m_is_initialized = true;
    }

    void step(double dt, const BrainSensoryInput& input, BrainMotorOutput& out) override {
        if (!m_is_initialized || m_somas.empty()) return;

        m_step_count++;

        // 1. Compute sensory drive for left and right hemispheres
        double left_visual_drive = 0.0;
        double right_visual_drive = 0.0;
        for (size_t s = 0; s < 8; ++s) {
            left_visual_drive += input.left_eye_sectors[s];
            right_visual_drive += input.right_eye_sectors[s];
        }
        left_visual_drive /= 8.0;
        right_visual_drive /= 8.0;

        double olfactory_drive = input.odor_sugar_intensity * 2.0 + input.odor_pheromone_intensity;
        double mechanosensory_drive = input.air_velocity.length() * 0.5;

        // 2. Update neural membrane potentials & firing rates
        // Leaky integration rate model
        double leak = std::exp(-dt / 0.02); // 20 ms membrane time constant
        double left_activation_sum = 0.0;
        double right_activation_sum = 0.0;

        // Left hemisphere integration
        for (size_t idx : m_left_somas) {
            double drive = left_visual_drive + olfactory_drive * 0.5 + mechanosensory_drive * 0.2;
            m_potentials[idx] = -65.0 + (m_potentials[idx] - (-65.0)) * leak + drive * 25.0 * dt;
            // Sigmoidal firing rate activation
            double act = 1.0 / (1.0 + std::exp(-(m_potentials[idx] - (-50.0)) / 5.0));
            m_activations[idx] = act;
            left_activation_sum += act;
        }

        // Right hemisphere integration
        for (size_t idx : m_right_somas) {
            double drive = right_visual_drive + olfactory_drive * 0.5 + mechanosensory_drive * 0.2;
            m_potentials[idx] = -65.0 + (m_potentials[idx] - (-65.0)) * leak + drive * 25.0 * dt;
            double act = 1.0 / (1.0 + std::exp(-(m_potentials[idx] - (-50.0)) / 5.0));
            m_activations[idx] = act;
            right_activation_sum += act;
        }

        double left_mean_act = m_left_somas.empty() ? 0.0 : (left_activation_sum / m_left_somas.size());
        double right_mean_act = m_right_somas.empty() ? 0.0 : (right_activation_sum / m_right_somas.size());

        // 3. Descending Motor Output Integration (Flight motor & Feeding)
        // Baseline wingbeat: 200 Hz
        double base_freq = 200.0;
        // Optomotor steering response: Differential wingbeat frequency
        double delta_act = left_mean_act - right_mean_act;
        out.left_wing_freq_hz = base_freq + (left_mean_act * 30.0) - (delta_act * 15.0);
        out.right_wing_freq_hz = base_freq + (right_mean_act * 30.0) + (delta_act * 15.0);
        out.wing_amplitude_deg = 135.0 + (left_mean_act + right_mean_act) * 10.0;

        // Steering torque yaw proportional to differential wingbeat
        out.steering_torque.y = delta_act * 2.0;

        // Proboscis Extension Response (PER) triggered by sugar gustatory sensation
        out.proboscis_extension = std::clamp(input.odor_sugar_intensity * 1.5, 0.0, 1.0);

        // Leg walking locomotion impulse
        if (out.left_wing_freq_hz < 150.0 || input.internal_energy < 10.0) {
            out.leg_locomotion_impulse = Vec3{0.0, 0.0, 0.5};
        } else {
            out.leg_locomotion_impulse = Vec3{0.0, 0.0, 0.0};
        }
    }

    [[nodiscard]] size_t soma_count() const noexcept override { return m_somas.size(); }
    [[nodiscard]] size_t active_neuron_count() const noexcept override {
        size_t active = 0;
        for (double act : m_activations) {
            if (act > 0.1) active++;
        }
        return active;
    }

    [[nodiscard]] const std::vector<NeuronSoma>& somas() const noexcept { return m_somas; }

    [[nodiscard]] uint64_t compute_brain_hash() const noexcept override {
        uint64_t h = 14695981039346656037ULL;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix(m_somas.size());
        mix(m_step_count);
        for (size_t i = 0; i < std::min<size_t>(m_activations.size(), 100); ++i) {
            uint64_t v = 0;
            std::memcpy(&v, &m_activations[i], sizeof(double));
            mix(v);
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        return {
            {"soma_count", m_somas.size()},
            {"left_count", m_left_somas.size()},
            {"right_count", m_right_somas.size()},
            {"steps", m_step_count},
            {"active_neurons", active_neuron_count()},
            {"activations", m_activations},
            {"potentials", m_potentials}
        };
    }

    void from_json(const nlohmann::json& j) override {
        if (j.contains("steps")) {
            m_step_count = j["steps"].get<uint64_t>();
        }
        if (j.contains("activations")) {
            m_activations = j["activations"].get<std::vector<double>>();
        }
        if (j.contains("potentials")) {
            m_potentials = j["potentials"].get<std::vector<double>>();
        }
    }
};

} // namespace flgod::brain
