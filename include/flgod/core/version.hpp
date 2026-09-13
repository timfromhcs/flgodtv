#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

namespace flgod {

struct SimulationVersion {
    uint32_t major{0};
    uint32_t minor{1};
    uint32_t patch{0};
    uint32_t schema_version{1};
    std::string build_meta{"local-alpha"};

    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << major << "." << minor << "." << patch;
        if (!build_meta.empty()) {
            oss << "-" << build_meta;
        }
        return oss.str();
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL; // FNV-1a offset basis
        auto hash_byte = [&h](uint8_t b) {
            h ^= b;
            h *= 1099511628211ULL;
        };
        auto hash_u32 = [&hash_byte](uint32_t v) {
            for (int i = 0; i < 4; ++i) {
                hash_byte(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
            }
        };
        hash_u32(major);
        hash_u32(minor);
        hash_u32(patch);
        hash_u32(schema_version);
        for (char c : build_meta) {
            hash_byte(static_cast<uint8_t>(c));
        }
        return h;
    }

    bool operator==(const SimulationVersion& other) const = default;
};

inline const SimulationVersion CURRENT_SIMULATION_VERSION{
    .major = 0,
    .minor = 1,
    .patch = 0,
    .schema_version = 1,
    .build_meta = "dev"
};

} // namespace flgod
