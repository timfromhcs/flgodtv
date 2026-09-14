#pragma once

// MPE core types: pipeline stages, versions, hashing helper.
// Species-agnostic. No Godot, no brain, no scenario assumptions.

#include <cstdint>
#include <string>

namespace flgod::mpe {

inline constexpr uint32_t MPE_API_VERSION = 1;
inline constexpr uint32_t MPE_COMPONENT_SCHEMA_VERSION = 1;
inline constexpr uint32_t MPE_SCENARIO_SCHEMA_VERSION = 1;
inline constexpr uint32_t MPE_TELEMETRY_SCHEMA_VERSION = 1;

// Deterministic pipeline order (mission Phase 10). Values are stable;
// persisted logs may record them.
enum class Stage : uint8_t {
    TickStart = 0,
    Environment,
    Perception,
    Brain,
    Decision,
    ActionValidation,
    Physics,
    Biology,
    Social,
    Learning,
    Reproduction,
    Evolution,
    Technology,
    Telemetry,
    Checkpoint,
    TickEnd,
    Count
};

[[nodiscard]] inline std::string stage_name(Stage s) {
    switch (s) {
        case Stage::TickStart: return "TickStart";
        case Stage::Environment: return "Environment";
        case Stage::Perception: return "Perception";
        case Stage::Brain: return "Brain";
        case Stage::Decision: return "Decision";
        case Stage::ActionValidation: return "ActionValidation";
        case Stage::Physics: return "Physics";
        case Stage::Biology: return "Biology";
        case Stage::Social: return "Social";
        case Stage::Learning: return "Learning";
        case Stage::Reproduction: return "Reproduction";
        case Stage::Evolution: return "Evolution";
        case Stage::Technology: return "Technology";
        case Stage::Telemetry: return "Telemetry";
        case Stage::Checkpoint: return "Checkpoint";
        case Stage::TickEnd: return "TickEnd";
        default: return "Unknown";
    }
}

// FNV-1a 64: stable, no external dependency.
[[nodiscard]] inline uint64_t fnv1a64(const std::string& s) noexcept {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

} // namespace flgod::mpe
