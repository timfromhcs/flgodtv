#pragma once

// Data-driven archetypes + scenarios with explicit validation.
// A scenario MUST be definable without touching C++ source.

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace flgod::mpe {

struct ArchetypePopulation {
    std::string archetype;
    uint64_t count{0};
};

struct Archetype {
    std::string name;
    nlohmann::json components = nlohmann::json::object();
    std::string brain_provider{"scripted"};
    nlohmann::json brain_config = nlohmann::json::object();
    std::vector<std::string> actions{"Rest"};
    std::vector<std::string> sensors;
    double spawn_radius{10.0};

    static Archetype load(const std::string& path) {
        Archetype a;
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("Archetype: cannot open " + path);
        }
        nlohmann::json j;
        try {
            f >> j;
        } catch (const std::exception& e) {
            throw std::runtime_error("Archetype: invalid JSON in " + path + ": " + e.what());
        }
        if (!j.contains("name") || !j["name"].is_string()) {
            throw std::runtime_error("Archetype: missing string 'name' in " + path);
        }
        a.name = j["name"].get<std::string>();
        if (j.contains("components")) {
            if (!j["components"].is_object()) {
                throw std::runtime_error("Archetype: 'components' must be an object in " + path);
            }
            a.components = j["components"];
        }
        if (j.contains("brain")) {
            const auto& b = j["brain"];
            if (!b.is_object() || !b.contains("provider") || !b["provider"].is_string()) {
                throw std::runtime_error("Archetype: 'brain.provider' must be a string in " + path);
            }
            a.brain_provider = b["provider"].get<std::string>();
            if (b.contains("config")) a.brain_config = b["config"];
        }
        if (j.contains("actions")) {
            if (!j["actions"].is_array()) {
                throw std::runtime_error("Archetype: 'actions' must be an array in " + path);
            }
            a.actions.clear();
            for (const auto& e : j["actions"]) {
                if (!e.is_string()) {
                    throw std::runtime_error("Archetype: 'actions' entries must be strings in " + path);
                }
                a.actions.push_back(e.get<std::string>());
            }
        }
        if (a.actions.empty()) {
            throw std::runtime_error("Archetype: 'actions' must not be empty in " + path);
        }
        if (j.contains("sensors")) {
            if (!j["sensors"].is_array()) {
                throw std::runtime_error("Archetype: 'sensors' must be an array in " + path);
            }
            for (const auto& e : j["sensors"]) {
                if (!e.is_string()) {
                    throw std::runtime_error("Archetype: 'sensors' entries must be strings in " + path);
                }
                a.sensors.push_back(e.get<std::string>());
            }
        }
        if (j.contains("spawn_radius")) {
            if (!j["spawn_radius"].is_number() || j["spawn_radius"].get<double>() < 0.0) {
                throw std::runtime_error("Archetype: 'spawn_radius' must be >= 0 in " + path);
            }
            a.spawn_radius = j["spawn_radius"].get<double>();
        }
        return a;
    }
};

struct ScenarioRule {
    std::string name;
    nlohmann::json config = nlohmann::json::object();
};

struct Scenario {
    std::string name;
    uint32_t scenario_version{1};
    std::string simulation_version{"0.1.0"};
    uint64_t master_seed{1};
    uint64_t ticks{100};
    std::vector<ArchetypePopulation> populations;
    std::vector<ScenarioRule> rules;
    nlohmann::json presentation = nlohmann::json::object();

    static Scenario load(const std::string& path) {
        Scenario s;
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("Scenario: cannot open " + path);
        }
        nlohmann::json j;
        try {
            f >> j;
        } catch (const std::exception& e) {
            throw std::runtime_error("Scenario: invalid JSON in " + path + ": " + e.what());
        }
        for (const char* key : {"name", "scenario_version", "master_seed"}) {
            if (!j.contains(key)) {
                throw std::runtime_error(std::string("Scenario: missing required '") + key + "' in " + path);
            }
        }
        if (!j["name"].is_string() || !j["scenario_version"].is_number_unsigned() ||
            !j["master_seed"].is_number_unsigned()) {
            throw std::runtime_error("Scenario: bad types for name/scenario_version/master_seed in " + path);
        }
        s.name = j["name"].get<std::string>();
        s.scenario_version = j["scenario_version"].get<uint32_t>();
        s.master_seed = j["master_seed"].get<uint64_t>();
        s.simulation_version = j.value("simulation_version", s.simulation_version);
        s.ticks = j.value("ticks", s.ticks);
        if (!j.contains("populations") || !j["populations"].is_array() || j["populations"].empty()) {
            throw std::runtime_error("Scenario: non-empty 'populations' array required in " + path);
        }
        for (const auto& p : j["populations"]) {
            if (!p.is_object() || !p.contains("archetype") || !p["archetype"].is_string() ||
                !p.contains("count") || !p["count"].is_number_unsigned()) {
                throw std::runtime_error("Scenario: each population needs string 'archetype' + unsigned 'count' in " + path);
            }
            s.populations.push_back(
                ArchetypePopulation{p["archetype"].get<std::string>(), p["count"].get<uint64_t>()});
        }
        if (j.contains("rules")) {
            if (!j["rules"].is_array()) {
                throw std::runtime_error("Scenario: 'rules' must be an array in " + path);
            }
            for (const auto& r : j["rules"]) {
                ScenarioRule sr;
                if (r.is_string()) {
                    sr.name = r.get<std::string>();
                } else if (r.is_object() && r.contains("name") && r["name"].is_string()) {
                    sr.name = r["name"].get<std::string>();
                    if (r.contains("config")) {
                        if (!r["config"].is_object()) {
                            throw std::runtime_error(
                                "Scenario: rule 'config' must be an object in " + path);
                        }
                        sr.config = r["config"];
                    }
                } else {
                    throw std::runtime_error(
                        "Scenario: 'rules' entries must be strings or {name, config} in " + path);
                }
                if (sr.name.empty()) {
                    throw std::runtime_error("Scenario: rule name must not be empty in " + path);
                }
                s.rules.push_back(std::move(sr));
            }
        }
        if (j.contains("presentation")) s.presentation = j["presentation"];
        return s;
    }
};

} // namespace flgod::mpe
