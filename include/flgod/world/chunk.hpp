#pragma once

#include "flgod/world/chunk_id.hpp"
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace flgod {

enum class BiomeType : uint8_t {
    Ocean = 0,
    Plains = 1,
    Forest = 2,
    Rainforest = 3,
    Desert = 4,
    Tundra = 5,
    Mountain = 6,
    Wetland = 7
};

inline const char* biome_to_string(BiomeType b) noexcept {
    switch (b) {
        case BiomeType::Ocean: return "Ocean";
        case BiomeType::Plains: return "Plains";
        case BiomeType::Forest: return "Forest";
        case BiomeType::Rainforest: return "Rainforest";
        case BiomeType::Desert: return "Desert";
        case BiomeType::Tundra: return "Tundra";
        case BiomeType::Mountain: return "Mountain";
        case BiomeType::Wetland: return "Wetland";
        default: return "Unknown";
    }
}

constexpr uint32_t CHUNK_SIZE = 16; // 16x16 cells per chunk
constexpr double CHUNK_CELL_SIZE = 1.0; // 1 meter per cell

struct ChunkDelta {
    std::unordered_map<uint16_t, double> height_deltas; // cell index -> height delta
    std::unordered_map<uint16_t, double> biomass_deltas;
    std::unordered_map<uint16_t, uint8_t> biome_overrides;

    [[nodiscard]] bool empty() const noexcept {
        return height_deltas.empty() && biomass_deltas.empty() && biome_overrides.empty();
    }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_u64 = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        for (const auto& [idx, dh] : height_deltas) {
            mix_u64(idx);
            uint64_t bits = 0;
            std::memcpy(&bits, &dh, sizeof(double));
            mix_u64(bits);
        }
        for (const auto& [idx, db] : biomass_deltas) {
            mix_u64(idx);
            uint64_t bits = 0;
            std::memcpy(&bits, &db, sizeof(double));
            mix_u64(bits);
        }
        for (const auto& [idx, bio] : biome_overrides) {
            mix_u64(idx);
            mix_u64(bio);
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        nlohmann::json h_arr = nlohmann::json::array();
        for (const auto& [idx, dh] : height_deltas) {
            h_arr.push_back({{"idx", idx}, {"val", dh}});
        }
        nlohmann::json b_arr = nlohmann::json::array();
        for (const auto& [idx, db] : biomass_deltas) {
            b_arr.push_back({{"idx", idx}, {"val", db}});
        }
        nlohmann::json o_arr = nlohmann::json::array();
        for (const auto& [idx, bio] : biome_overrides) {
            o_arr.push_back({{"idx", idx}, {"val", bio}});
        }
        j["heights"] = h_arr;
        j["biomass"] = b_arr;
        j["biomes"] = o_arr;
        return j;
    }

    void from_json(const nlohmann::json& j) {
        height_deltas.clear();
        biomass_deltas.clear();
        biome_overrides.clear();
        if (j.contains("heights") && j["heights"].is_array()) {
            for (const auto& item : j["heights"]) {
                uint16_t idx = item.value("idx", static_cast<uint16_t>(0));
                double val = item.value("val", 0.0);
                height_deltas[idx] = val;
            }
        }
        if (j.contains("biomass") && j["biomass"].is_array()) {
            for (const auto& item : j["biomass"]) {
                uint16_t idx = item.value("idx", static_cast<uint16_t>(0));
                double val = item.value("val", 0.0);
                biomass_deltas[idx] = val;
            }
        }
        if (j.contains("biomes") && j["biomes"].is_array()) {
            for (const auto& item : j["biomes"]) {
                uint16_t idx = item.value("idx", static_cast<uint16_t>(0));
                uint8_t val = item.value("val", static_cast<uint8_t>(0));
                biome_overrides[idx] = val;
            }
        }
    }
};

class WorldChunk {
public:
    WorldChunk() = default;
    WorldChunk(ChunkCoord coord, uint64_t chunk_seed)
        : m_coord(coord),
          m_id(coord_to_chunk_id(coord)),
          m_seed(chunk_seed) {
        m_base_heights.fill(0.0);
        m_base_biomes.fill(BiomeType::Plains);
        m_base_moisture.fill(0.5);
        m_base_water_height.fill(0.0);
    }

    [[nodiscard]] const ChunkCoord& coord() const noexcept { return m_coord; }
    [[nodiscard]] ChunkID id() const noexcept { return m_id; }
    [[nodiscard]] uint64_t seed() const noexcept { return m_seed; }

    [[nodiscard]] double get_height(uint32_t lx, uint32_t lz) const noexcept {
        uint16_t idx = cell_index(lx, lz);
        double h = m_base_heights[idx];
        auto it = m_delta.height_deltas.find(idx);
        if (it != m_delta.height_deltas.end()) {
            h += it->second;
        }
        return h;
    }

    void set_base_height(uint32_t lx, uint32_t lz, double h) noexcept {
        m_base_heights[cell_index(lx, lz)] = h;
    }

    [[nodiscard]] BiomeType get_biome(uint32_t lx, uint32_t lz) const noexcept {
        uint16_t idx = cell_index(lx, lz);
        auto it = m_delta.biome_overrides.find(idx);
        if (it != m_delta.biome_overrides.end()) {
            return static_cast<BiomeType>(it->second);
        }
        return m_base_biomes[idx];
    }

    void set_base_biome(uint32_t lx, uint32_t lz, BiomeType b) noexcept {
        m_base_biomes[cell_index(lx, lz)] = b;
    }

    [[nodiscard]] double get_moisture(uint32_t lx, uint32_t lz) const noexcept {
        return m_base_moisture[cell_index(lx, lz)];
    }

    void set_base_moisture(uint32_t lx, uint32_t lz, double m) noexcept {
        m_base_moisture[cell_index(lx, lz)] = m;
    }

    [[nodiscard]] double get_water_height(uint32_t lx, uint32_t lz) const noexcept {
        return m_base_water_height[cell_index(lx, lz)];
    }

    void set_base_water_height(uint32_t lx, uint32_t lz, double wh) noexcept {
        m_base_water_height[cell_index(lx, lz)] = wh;
    }

    void apply_height_delta(uint32_t lx, uint32_t lz, double delta) noexcept {
        m_delta.height_deltas[cell_index(lx, lz)] += delta;
    }

    void set_biome_override(uint32_t lx, uint32_t lz, BiomeType b) noexcept {
        m_delta.biome_overrides[cell_index(lx, lz)] = static_cast<uint8_t>(b);
    }

    [[nodiscard]] const ChunkDelta& delta() const noexcept { return m_delta; }
    ChunkDelta& delta() noexcept { return m_delta; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix(m_id);
        mix(m_seed);
        for (double val : m_base_heights) {
            uint64_t bits = 0;
            std::memcpy(&bits, &val, sizeof(double));
            mix(bits);
        }
        for (BiomeType b : m_base_biomes) {
            mix(static_cast<uint64_t>(b));
        }
        mix(m_delta.compute_hash());
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"coord", {{"x", m_coord.x}, {"y", m_coord.y}, {"z", m_coord.z}}},
            {"id", m_id},
            {"seed", m_seed},
            {"delta", m_delta.to_json()}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("coord")) {
            m_coord.x = j["coord"].value("x", 0);
            m_coord.y = j["coord"].value("y", 0);
            m_coord.z = j["coord"].value("z", 0);
            m_id = coord_to_chunk_id(m_coord);
        }
        m_seed = j.value("seed", 0ULL);
        if (j.contains("delta")) {
            m_delta.from_json(j["delta"]);
        }
    }

private:
    static constexpr uint16_t cell_index(uint32_t lx, uint32_t lz) noexcept {
        return static_cast<uint16_t>((lz % CHUNK_SIZE) * CHUNK_SIZE + (lx % CHUNK_SIZE));
    }

    ChunkCoord m_coord{};
    ChunkID m_id{0};
    uint64_t m_seed{0};

    std::array<double, CHUNK_SIZE * CHUNK_SIZE> m_base_heights{};
    std::array<BiomeType, CHUNK_SIZE * CHUNK_SIZE> m_base_biomes{};
    std::array<double, CHUNK_SIZE * CHUNK_SIZE> m_base_moisture{};
    std::array<double, CHUNK_SIZE * CHUNK_SIZE> m_base_water_height{};

    ChunkDelta m_delta;
};

} // namespace flgod
