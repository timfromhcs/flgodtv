#pragma once

#include "flgod/world/chunk_id.hpp"
#include "flgod/world/chunk.hpp"
#include "flgod/world/noise.hpp"
#include "flgod/world/fields.hpp"
#include "flgod/world/procedural_generator.hpp"
#include "flgod/world/weather_system.hpp"
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace flgod {

struct WorldConfig {
    uint64_t world_seed{133701ULL};
    uint64_t weather_seed{133702ULL};
    ProceduralGenConfig gen_config{};
    uint32_t fields_width{64};
    uint32_t fields_height{64};
    double fields_cell_size{2.0};
};

class World {
public:
    World() : World(WorldConfig{}) {}
    explicit World(const WorldConfig& config)
        : m_config(config),
          m_generator(config.world_seed, config.gen_config),
          m_weather(config.weather_seed),
          m_wind(config.fields_width, config.fields_height, config.fields_cell_size),
          m_temperature(config.fields_width, config.fields_height, config.fields_cell_size, 0.0, 0.0, 20.0),
          m_humidity(config.fields_width, config.fields_height, config.fields_cell_size, 0.0, 0.0, 0.5),
          m_moisture(config.fields_width, config.fields_height, config.fields_cell_size, 0.0, 0.0, 0.5),
          m_water(config.fields_width, config.fields_height, config.fields_cell_size),
          m_fire(config.fields_width, config.fields_height, config.fields_cell_size),
          m_ecology(config.fields_width, config.fields_height, config.fields_cell_size) {}

    [[nodiscard]] const WorldConfig& config() const noexcept { return m_config; }
    [[nodiscard]] const WeatherSystem& weather() const noexcept { return m_weather; }
    WeatherSystem& weather() noexcept { return m_weather; }

    [[nodiscard]] const WindField& wind() const noexcept { return m_wind; }
    WindField& wind() noexcept { return m_wind; }

    [[nodiscard]] const TemperatureField& temperature() const noexcept { return m_temperature; }
    TemperatureField& temperature() noexcept { return m_temperature; }

    [[nodiscard]] const HumidityField& humidity() const noexcept { return m_humidity; }
    HumidityField& humidity() noexcept { return m_humidity; }

    [[nodiscard]] const MoistureField& moisture() const noexcept { return m_moisture; }
    MoistureField& moisture() noexcept { return m_moisture; }

    [[nodiscard]] const WaterField& water() const noexcept { return m_water; }
    WaterField& water() noexcept { return m_water; }

    [[nodiscard]] const FireField& fire() const noexcept { return m_fire; }
    FireField& fire() noexcept { return m_fire; }

    [[nodiscard]] const EcologyField& ecology() const noexcept { return m_ecology; }
    EcologyField& ecology() noexcept { return m_ecology; }

    [[nodiscard]] double sample_temperature(double x, double z) const noexcept {
        return m_temperature.sample(x, z);
    }

    [[nodiscard]] double sample_humidity(double x, double z) const noexcept {
        return m_humidity.sample(x, z);
    }

    WorldChunk& get_or_create_chunk(const ChunkCoord& coord) {
        auto it = m_chunks.find(coord);
        if (it != m_chunks.end()) {
            return it->second;
        }
        WorldChunk chunk = m_generator.generate_chunk(coord);
        auto inserted = m_chunks.emplace(coord, std::move(chunk));
        return inserted.first->second;
    }

    [[nodiscard]] bool has_chunk(const ChunkCoord& coord) const noexcept {
        return m_chunks.find(coord) != m_chunks.end();
    }

    [[nodiscard]] size_t loaded_chunk_count() const noexcept {
        return m_chunks.size();
    }

    // Continuous world elevation sampling across chunk boundaries
    [[nodiscard]] double sample_elevation(double wx, double wz) {
        int32_t cx = static_cast<int32_t>(std::floor(wx / (CHUNK_SIZE * CHUNK_CELL_SIZE)));
        int32_t cz = static_cast<int32_t>(std::floor(wz / (CHUNK_SIZE * CHUNK_CELL_SIZE)));
        ChunkCoord coord{.x = cx, .y = 0, .z = cz};

        WorldChunk& chunk = get_or_create_chunk(coord);

        double local_x = wx - cx * (CHUNK_SIZE * CHUNK_CELL_SIZE);
        double local_z = wz - cz * (CHUNK_SIZE * CHUNK_CELL_SIZE);

        uint32_t lx = static_cast<uint32_t>(std::clamp(std::floor(local_x / CHUNK_CELL_SIZE), 0.0, static_cast<double>(CHUNK_SIZE - 1)));
        uint32_t lz = static_cast<uint32_t>(std::clamp(std::floor(local_z / CHUNK_CELL_SIZE), 0.0, static_cast<double>(CHUNK_SIZE - 1)));

        return chunk.get_height(lx, lz);
    }

    // Biome query
    [[nodiscard]] BiomeType sample_biome(double wx, double wz) {
        int32_t cx = static_cast<int32_t>(std::floor(wx / (CHUNK_SIZE * CHUNK_CELL_SIZE)));
        int32_t cz = static_cast<int32_t>(std::floor(wz / (CHUNK_SIZE * CHUNK_CELL_SIZE)));
        ChunkCoord coord{.x = cx, .y = 0, .z = cz};

        WorldChunk& chunk = get_or_create_chunk(coord);

        double local_x = wx - cx * (CHUNK_SIZE * CHUNK_CELL_SIZE);
        double local_z = wz - cz * (CHUNK_SIZE * CHUNK_CELL_SIZE);

        uint32_t lx = static_cast<uint32_t>(std::clamp(std::floor(local_x / CHUNK_CELL_SIZE), 0.0, static_cast<double>(CHUNK_SIZE - 1)));
        uint32_t lz = static_cast<uint32_t>(std::clamp(std::floor(local_z / CHUNK_CELL_SIZE), 0.0, static_cast<double>(CHUNK_SIZE - 1)));

        return chunk.get_biome(lx, lz);
    }

    void step(double dt, double elapsed_seconds) {
        // 1. Advance weather system
        m_weather.step(dt, elapsed_seconds);

        // 2. Synchronize weather effects into continuous fields
        const WeatherState& ws = m_weather.state();
        for (uint32_t y = 0; y < m_temperature.height(); ++y) {
            for (uint32_t x = 0; x < m_temperature.width(); ++x) {
                m_temperature.set_grid(x, y, ws.temperature);
                m_humidity.set_grid(x, y, ws.humidity);
                m_wind.set(x, y, ws.wind);
                if (ws.precipitation > 0.01) {
                    double cur_m = m_moisture.get_grid(x, y);
                    m_moisture.set_grid(x, y, std::min(1.0, cur_m + ws.precipitation * 0.1 * dt));
                }
            }
        }

        // 3. Step fire and ecology fields
        m_fire.step_fire(dt, m_wind, m_moisture);
        m_ecology.step_ecology(dt, m_temperature, m_moisture);
    }

    [[nodiscard]] uint64_t compute_world_hash() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto mix_u64 = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix_u64(m_config.world_seed);
        mix_u64(m_config.weather_seed);
        mix_u64(m_weather.compute_hash());
        mix_u64(m_wind.compute_hash());
        mix_u64(m_temperature.compute_hash());
        mix_u64(m_moisture.compute_hash());
        mix_u64(m_fire.compute_hash());
        mix_u64(m_ecology.compute_hash());

        // Sort chunks by coordinate for deterministic hashing order
        std::vector<ChunkCoord> coords;
        coords.reserve(m_chunks.size());
        for (const auto& [coord, _] : m_chunks) {
            coords.push_back(coord);
        }
        std::sort(coords.begin(), coords.end());

        for (const auto& c : coords) {
            mix_u64(m_chunks.at(c).compute_hash());
        }
        return h;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j;
        j["world_seed"] = m_config.world_seed;
        j["weather_seed"] = m_config.weather_seed;
        j["weather"] = m_weather.to_json();
        j["wind"] = m_wind.to_json();
        j["temperature"] = m_temperature.to_json();
        j["humidity"] = m_humidity.to_json();
        j["moisture"] = m_moisture.to_json();
        j["water"] = m_water.to_json();
        j["fire"] = m_fire.to_json();
        j["ecology"] = m_ecology.to_json();

        nlohmann::json chunk_array = nlohmann::json::array();
        for (const auto& [coord, chunk] : m_chunks) {
            chunk_array.push_back(chunk.to_json());
        }
        j["chunks"] = chunk_array;
        j["world_hash"] = compute_world_hash();
        return j;
    }

    void from_json(const nlohmann::json& j) {
        m_config.world_seed = j.value("world_seed", 133701ULL);
        m_config.weather_seed = j.value("weather_seed", 133702ULL);
        m_generator = ProceduralGenerator(m_config.world_seed, m_config.gen_config);
        if (j.contains("weather")) m_weather.from_json(j["weather"]);
        if (j.contains("wind")) m_wind.from_json(j["wind"]);
        if (j.contains("temperature")) m_temperature.from_json(j["temperature"]);
        if (j.contains("humidity")) m_humidity.from_json(j["humidity"]);
        if (j.contains("moisture")) m_moisture.from_json(j["moisture"]);
        if (j.contains("water")) m_water.from_json(j["water"]);
        if (j.contains("fire")) m_fire.from_json(j["fire"]);
        if (j.contains("ecology")) m_ecology.from_json(j["ecology"]);

        m_chunks.clear();
        if (j.contains("chunks")) {
            for (const auto& cj : j["chunks"]) {
                ChunkCoord coord{
                    .x = cj["coord"].value("x", 0),
                    .y = cj["coord"].value("y", 0),
                    .z = cj["coord"].value("z", 0)
                };
                WorldChunk chunk = m_generator.generate_chunk(coord);
                chunk.from_json(cj);
                m_chunks.emplace(coord, std::move(chunk));
            }
        }
    }

private:
    WorldConfig m_config;
    ProceduralGenerator m_generator;
    WeatherSystem m_weather;

    WindField m_wind;
    TemperatureField m_temperature;
    HumidityField m_humidity;
    MoistureField m_moisture;
    WaterField m_water;
    FireField m_fire;
    EcologyField m_ecology;

    std::unordered_map<ChunkCoord, WorldChunk> m_chunks;
};

} // namespace flgod
