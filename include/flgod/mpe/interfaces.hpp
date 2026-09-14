#pragma once

// Generic MPE interfaces: sensor, brain, environment module, rule, scheduler.
// Core depends only on these; species logic lives in adapters/scenarios.

#include "flgod/core/entity_id.hpp"
#include "flgod/mpe/mpe_types.hpp"
#include "flgod/mpe/observation.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace flgod::mpe {

class EntityRegistry; // defined in entity.hpp (included by users as needed)

// ---- Sensor: Environment + Entity -> Observation. MUST NOT mutate state. ----
class ISensor {
public:
    virtual ~ISensor() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual Observation sample(uint64_t entity_raw,
                                            const nlohmann::json& env_view) const = 0;
};

// ---- Brain: Observation -> ActionIntent. MUST NOT mutate world state. ----
class IBrain {
public:
    virtual ~IBrain() = default;
    [[nodiscard]] virtual std::string provider() const = 0;
    virtual void initialize(const nlohmann::json& config) = 0;
    virtual void observe(const Observation& obs) = 0;
    virtual void step(double dt) = 0;
    [[nodiscard]] virtual std::vector<ActionIntent> intents() const = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
    [[nodiscard]] virtual uint64_t compute_hash() const = 0;
    virtual void reset() = 0;
};

// Deterministic scripted brain (tests, simple fauna).
class ScriptedBrain : public IBrain {
public:
    [[nodiscard]] std::string provider() const override { return "scripted"; }
    void initialize(const nlohmann::json& config) override {
        m_action = config.value("action", std::string{"Rest"});
    }
    void observe(const Observation& obs) override { m_last = obs; }
    void step(double) override { m_steps++; }
    [[nodiscard]] std::vector<ActionIntent> intents() const override {
        ActionIntent in;
        in.action = m_action;
        in.priority = 1;
        return {in};
    }
    [[nodiscard]] nlohmann::json to_json() const override {
        return {{"provider", "scripted"}, {"action", m_action}, {"steps", m_steps}};
    }
    void from_json(const nlohmann::json& j) override {
        m_action = j.value("action", std::string{"Rest"});
        m_steps = j.value("steps", uint64_t{0});
    }
    [[nodiscard]] uint64_t compute_hash() const override {
        return fnv1a64("scripted:" + m_action + ":" + std::to_string(m_steps));
    }
    void reset() override {
        m_steps = 0;
        m_last = Observation{};
    }

private:
    std::string m_action{"Rest"};
    Observation m_last;
    uint64_t m_steps{0};
};

// ---- Environment module: deterministic lifecycle + serialization. ----
class IEnvironmentModule {
public:
    virtual ~IEnvironmentModule() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual uint32_t version() const {
        return MPE_COMPONENT_SCHEMA_VERSION;
    }
    virtual void initialize(const nlohmann::json& params, uint64_t seed) = 0;
    virtual void step(double dt, uint64_t tick) = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
    [[nodiscard]] virtual uint64_t compute_hash() const = 0;
    [[nodiscard]] virtual bool enabled() const { return m_enabled; }
    virtual void set_enabled(bool e) { m_enabled = e; }

private:
    bool m_enabled{true};
};

// ---- Rule: see rules.hpp for the full replaceable rule layer. ----

// ---- Scheduler: explicit, inspectable, deterministic system order. ----
class SimulationScheduler {
public:
    using SystemFn = std::function<void(Stage, uint64_t tick, double dt)>;
    void register_system(Stage stage, const std::string& name, SystemFn fn,
                         bool enabled = true) {
        for (const auto& s : m_systems) {
            if (s.name == name) {
                throw std::runtime_error("SimulationScheduler: duplicate system " + name);
            }
        }
        m_systems.push_back(SystemEntry{stage, name, std::move(fn), enabled});
    }
    void set_enabled(const std::string& name, bool enabled) {
        for (auto& s : m_systems) {
            if (s.name == name) {
                s.enabled = enabled;
                return;
            }
        }
        throw std::runtime_error("SimulationScheduler: unknown system " + name);
    }
    // Executes in Stage order; registration order breaks ties deterministically.
    void step(uint64_t tick, double dt) {
        // Stable insertion sort by stage (registration order preserved).
        for (size_t i = 1; i < m_systems.size(); ++i) {
            SystemEntry e = m_systems[i];
            size_t j = i;
            while (j > 0 && m_systems[j - 1].stage > e.stage) {
                m_systems[j] = m_systems[j - 1];
                --j;
            }
            m_systems[j] = e;
        }
        for (auto& s : m_systems) {
            if (s.enabled) s.fn(s.stage, tick, dt);
        }
    }
    [[nodiscard]] std::vector<std::string> order() const {
        std::vector<std::string> out;
        for (const auto& s : m_systems) {
            out.push_back(stage_name(s.stage) + ":" + s.name +
                          (s.enabled ? "" : " (disabled)"));
        }
        return out;
    }

private:
    struct SystemEntry {
        Stage stage;
        std::string name;
        SystemFn fn;
        bool enabled;
    };
    std::vector<SystemEntry> m_systems;
};

} // namespace flgod::mpe
