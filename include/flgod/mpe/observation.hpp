#pragma once

// Generic observation / action-intent data plane.
// Plain data only: no logic, no species assumptions.

#include <cstdint>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace flgod::mpe {

// Observation: named scalar channels + named string tags produced by sensors.
struct Observation {
    std::map<std::string, double> channels; // sorted => deterministic
    std::map<std::string, std::string> tags;

    void set(const std::string& k, double v) { channels[k] = v; }
    void tag(const std::string& k, const std::string& v) { tags[k] = v; }
    [[nodiscard]] double get(const std::string& k, double fallback = 0.0) const {
        auto it = channels.find(k);
        return it != channels.end() ? it->second : fallback;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"channels", channels}, {"tags", tags}};
    }
    void from_json(const nlohmann::json& j) {
        channels.clear();
        tags.clear();
        if (j.contains("channels")) {
            for (auto& [k, v] : j["channels"].items()) channels[k] = v.get<double>();
        }
        if (j.contains("tags")) {
            for (auto& [k, v] : j["tags"].items()) tags[k] = v.get<std::string>();
        }
    }
};

// ActionIntent: a brain's request. Executed only after validation.
struct ActionIntent {
    std::string action;   // e.g. "Move", "Eat", "Communicate", "Custom..."
    uint64_t entity_raw{0};
    std::map<std::string, double> params;
    uint64_t priority{0};

    [[nodiscard]] nlohmann::json to_json() const {
        return {{"action", action}, {"entity_raw", entity_raw},
                {"params", params}, {"priority", priority}};
    }
    void from_json(const nlohmann::json& j) {
        action = j.value("action", std::string{});
        entity_raw = j.value("entity_raw", uint64_t{0});
        params.clear();
        if (j.contains("params")) {
            for (auto& [k, v] : j["params"].items()) params[k] = v.get<double>();
        }
        priority = j.value("priority", uint64_t{0});
    }
};

struct ValidationResult {
    bool accepted{false};
    std::string reason;
};

} // namespace flgod::mpe
