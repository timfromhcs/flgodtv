#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <functional>

namespace flgod {

struct ChunkCoord {
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};

    constexpr bool operator==(const ChunkCoord& other) const noexcept = default;
    constexpr auto operator<=>(const ChunkCoord& other) const noexcept = default;

    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "(" << x << ", " << y << ", " << z << ")";
        return oss.str();
    }
};

using ChunkID = uint64_t;

// Deterministic chunk seed derivation: world_seed + coords + generator_version
inline uint64_t compute_chunk_seed(uint64_t world_seed, const ChunkCoord& coord, uint32_t generator_version = 1) noexcept {
    uint64_t h = world_seed ^ (static_cast<uint64_t>(generator_version) * 0x9E3779B97F4A7C15ULL);
    auto mix = [&h](uint64_t k) {
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 32;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 32;
        h ^= k;
        h = (h << 31) | (h >> 33);
        h = h * 5 + 0x52dce729;
    };
    mix(static_cast<uint64_t>(static_cast<int64_t>(coord.x)));
    mix(static_cast<uint64_t>(static_cast<int64_t>(coord.y)));
    mix(static_cast<uint64_t>(static_cast<int64_t>(coord.z)));
    return h;
}

// Convert ChunkCoord to a compact 64-bit ChunkID
inline ChunkID coord_to_chunk_id(const ChunkCoord& coord) noexcept {
    // 21 bits for x, 22 bits for y, 21 bits for z
    uint64_t ux = static_cast<uint64_t>(static_cast<int64_t>(coord.x) & 0x1FFFFF);
    uint64_t uy = static_cast<uint64_t>(static_cast<int64_t>(coord.y) & 0x3FFFFF);
    uint64_t uz = static_cast<uint64_t>(static_cast<int64_t>(coord.z) & 0x1FFFFF);
    return (ux << 43) | (uy << 21) | uz;
}

inline ChunkCoord chunk_id_to_coord(ChunkID id) noexcept {
    int64_t sx = static_cast<int64_t>((id >> 43) & 0x1FFFFF);
    int64_t sy = static_cast<int64_t>((id >> 21) & 0x3FFFFF);
    int64_t sz = static_cast<int64_t>(id & 0x1FFFFF);
    // Sign-extend from 21 and 22 bits
    if (sx & (1ULL << 20)) sx |= ~0x1FFFFFULL;
    if (sy & (1ULL << 21)) sy |= ~0x3FFFFFULL;
    if (sz & (1ULL << 20)) sz |= ~0x1FFFFFULL;
    return ChunkCoord{
        .x = static_cast<int32_t>(sx),
        .y = static_cast<int32_t>(sy),
        .z = static_cast<int32_t>(sz)
    };
}

} // namespace flgod

template <>
struct std::hash<flgod::ChunkCoord> {
    std::size_t operator()(const flgod::ChunkCoord& c) const noexcept {
        return std::hash<uint64_t>{}(flgod::coord_to_chunk_id(c));
    }
};
