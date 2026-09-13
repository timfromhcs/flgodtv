#pragma once

#include "flgod/world/chunk.hpp"
#include "flgod/world/noise.hpp"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace flgod {

struct ProceduralGenConfig {
    uint32_t version{1};
    double sea_level{0.0};
    double height_scale{32.0}; // Max mountain height variance
    double terrain_frequency{0.005};
    double moisture_frequency{0.003};
    double temperature_frequency{0.002};
    int terrain_octaves{5};
};

class ProceduralGenerator {
public:
    explicit ProceduralGenerator(uint64_t world_seed, const ProceduralGenConfig& config = ProceduralGenConfig{})
        : m_world_seed(world_seed),
          m_config(config),
          m_terrain_noise(world_seed ^ 0x1111222233334444ULL),
          m_moisture_noise(world_seed ^ 0x5555666677778888ULL),
          m_temp_noise(world_seed ^ 0x9999AAAABBBBCCCCULL),
          m_detail_noise(world_seed ^ 0xDDDDEEEEFFFF0000ULL) {}

    [[nodiscard]] WorldChunk generate_chunk(const ChunkCoord& coord) const {
        uint64_t chunk_seed = compute_chunk_seed(m_world_seed, coord, m_config.version);
        WorldChunk chunk(coord, chunk_seed);

        double origin_wx = static_cast<double>(coord.x * CHUNK_SIZE);
        double origin_wz = static_cast<double>(coord.z * CHUNK_SIZE);

        // Stage 1: Terrain Elevation & Micro-relief
        for (uint32_t lz = 0; lz < CHUNK_SIZE; ++lz) {
            for (uint32_t lx = 0; lx < CHUNK_SIZE; ++lx) {
                double wx = origin_wx + lx * CHUNK_CELL_SIZE;
                double wz = origin_wz + lz * CHUNK_CELL_SIZE;

                // Multi-octave fBm base
                double base = m_terrain_noise.fbm2d(
                    wx * m_config.terrain_frequency,
                    wz * m_config.terrain_frequency,
                    m_config.terrain_octaves
                );

                // Mountain ridges
                double ridge = m_terrain_noise.ridged2d(
                    wx * m_config.terrain_frequency * 1.5,
                    wz * m_config.terrain_frequency * 1.5,
                    3
                );

                // Combine: base + mountain shaping
                double raw_h = (base * 0.7 + ridge * 0.5) * m_config.height_scale;
                chunk.set_base_height(lx, lz, raw_h);
            }
        }

        // Stage 2: Thermal Erosion Approximation (neighbor slope smoothing)
        for (uint32_t lz = 1; lz < CHUNK_SIZE - 1; ++lz) {
            for (uint32_t lx = 1; lx < CHUNK_SIZE - 1; ++lx) {
                double h = chunk.get_height(lx, lz);
                double n_avg = (chunk.get_height(lx - 1, lz) + chunk.get_height(lx + 1, lz) +
                                chunk.get_height(lx, lz - 1) + chunk.get_height(lx, lz + 1)) * 0.25;
                double diff = h - n_avg;
                if (std::abs(diff) > 2.0) {
                    chunk.set_base_height(lx, lz, h - diff * 0.15); // Talus slope deposition
                }
            }
        }

        // Stage 3 & 4: Climate (Temperature, Humidity) & Hydrology
        for (uint32_t lz = 0; lz < CHUNK_SIZE; ++lz) {
            for (uint32_t lx = 0; lx < CHUNK_SIZE; ++lx) {
                double wx = origin_wx + lx * CHUNK_CELL_SIZE;
                double wz = origin_wz + lz * CHUNK_CELL_SIZE;
                double h = chunk.get_height(lx, lz);

                // Moisture: noise + proximity to sea level
                double moist_n = m_moisture_noise.fbm2d(
                    wx * m_config.moisture_frequency,
                    wz * m_config.moisture_frequency,
                    3
                );
                double moisture = std::clamp(moist_n * 0.5 + 0.5, 0.0, 1.0);
                if (h <= m_config.sea_level) {
                    moisture = 1.0;
                    chunk.set_base_water_height(lx, lz, m_config.sea_level);
                } else if (h <= m_config.sea_level + 2.0) {
                    moisture = std::min(1.0, moisture + 0.3); // Riverbank / coastal
                }
                chunk.set_base_moisture(lx, lz, moisture);

                // Temperature: base latitude + lapse rate with altitude
                double temp_n = m_temp_noise.noise2d(
                    wx * m_config.temperature_frequency,
                    wz * m_config.temperature_frequency
                );
                double base_temp = 20.0 + temp_n * 10.0; // 10C to 30C baseline
                double lapse = (h > 0.0) ? (h * 0.4) : 0.0; // Cools down with elevation
                double local_temp = base_temp - lapse;

                // Stage 5: Biome Classification (Whittaker Diagram)
                BiomeType biome = BiomeType::Plains;
                if (h <= m_config.sea_level) {
                    biome = BiomeType::Ocean;
                } else if (h > 24.0) {
                    biome = BiomeType::Mountain;
                } else if (local_temp < 2.0) {
                    biome = BiomeType::Tundra;
                } else if (local_temp > 25.0 && moisture < 0.25) {
                    biome = BiomeType::Desert;
                } else if (local_temp > 22.0 && moisture > 0.70) {
                    biome = BiomeType::Rainforest;
                } else if (moisture > 0.55 && h <= m_config.sea_level + 3.0) {
                    biome = BiomeType::Wetland;
                } else if (moisture > 0.50) {
                    biome = BiomeType::Forest;
                } else {
                    biome = BiomeType::Plains;
                }
                chunk.set_base_biome(lx, lz, biome);
            }
        }

        return chunk;
    }

    [[nodiscard]] uint64_t world_seed() const noexcept { return m_world_seed; }
    [[nodiscard]] const ProceduralGenConfig& config() const noexcept { return m_config; }

private:
    uint64_t m_world_seed;
    ProceduralGenConfig m_config;
    DeterministicNoise m_terrain_noise;
    DeterministicNoise m_moisture_noise;
    DeterministicNoise m_temp_noise;
    DeterministicNoise m_detail_noise;
};

} // namespace flgod
