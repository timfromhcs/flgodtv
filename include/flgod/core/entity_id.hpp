#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>

namespace flgod {

enum class EntityType : uint8_t {
    Unspecified = 0,
    Agent = 1,
    GodFly = 2,
    WorldObject = 3,
    Structure = 4,
    Resource = 5,
    Colony = 6,
    Marker = 7,
    Flora = 8,
    Fauna = 9
};

class EntityID {
public:
    constexpr EntityID() noexcept : m_id(0) {}
    constexpr explicit EntityID(uint64_t raw_id) noexcept : m_id(raw_id) {}
    
    constexpr EntityID(EntityType type, uint16_t generation, uint64_t index) noexcept {
        m_id = (static_cast<uint64_t>(type) << 56) |
               (static_cast<uint64_t>(generation) << 40) |
               (index & 0x000000FFFFFFFFFFULL);
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return m_id != 0;
    }

    [[nodiscard]] constexpr uint64_t raw() const noexcept {
        return m_id;
    }

    [[nodiscard]] constexpr EntityType type() const noexcept {
        return static_cast<EntityType>((m_id >> 56) & 0xFF);
    }

    [[nodiscard]] constexpr uint16_t generation() const noexcept {
        return static_cast<uint16_t>((m_id >> 40) & 0xFFFF);
    }

    [[nodiscard]] constexpr uint64_t index() const noexcept {
        return m_id & 0x000000FFFFFFFFFFULL;
    }

    [[nodiscard]] std::string to_string() const {
        if (!is_valid()) return "EntityID(NULL)";
        std::ostringstream oss;
        oss << "EntityID(type=" << static_cast<uint32_t>(type())
            << ", gen=" << generation()
            << ", idx=" << index() << ")";
        return oss.str();
    }

    constexpr bool operator==(const EntityID& other) const noexcept = default;
    constexpr auto operator<=>(const EntityID& other) const noexcept = default;

private:
    uint64_t m_id{0};
};

inline constexpr EntityID NULL_ENTITY{0};

class EntityIDAllocator {
public:
    explicit EntityIDAllocator(uint64_t initial_index = 1)
        : m_next_index(initial_index) {}

    void reset(uint64_t next_idx = 1, uint16_t gen = 1) noexcept {
        m_next_index = next_idx;
        m_current_generation = gen;
    }

    EntityID allocate(EntityType type = EntityType::Unspecified) noexcept {
        uint64_t idx = m_next_index++;
        return EntityID(type, m_current_generation, idx);
    }

    void advance_generation() noexcept {
        m_current_generation++;
    }

    [[nodiscard]] uint64_t next_index() const noexcept { return m_next_index; }
    [[nodiscard]] uint16_t current_generation() const noexcept { return m_current_generation; }

    [[nodiscard]] uint64_t compute_hash() const noexcept {
        return (m_next_index * 1099511628211ULL) ^ m_current_generation;
    }

private:
    uint64_t m_next_index{1};
    uint16_t m_current_generation{1};
};

} // namespace flgod

template <>
struct std::hash<flgod::EntityID> {
    std::size_t operator()(const flgod::EntityID& id) const noexcept {
        return std::hash<uint64_t>{}(id.raw());
    }
};
