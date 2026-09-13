#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "flgod/core/entity_id.hpp"
#include "flgod/world/fields.hpp"

namespace flgod::technology {

// SECTION 58: "Primitive capabilities: inspect, manipulate, combine, connect, operate, construct"
enum class PrimitiveCapability : uint8_t {
    Inspect = 1 << 0,     // Query object properties, durability, material
    Manipulate = 1 << 1,  // Pick up, carry, rotate, align
    Combine = 1 << 2,     // Merge items via recipe into composite tool
    Connect = 1 << 3,     // Establish structural or functional constraint/link
    Operate = 1 << 4,     // Trigger mechanism (lever, button, rotary shaft)
    Construct = 1 << 5    // Assemble into macro-structures (wall, nest, bridge)
};

inline constexpr uint8_t operator|(PrimitiveCapability a, PrimitiveCapability b) noexcept {
    return static_cast<uint8_t>(a) | static_cast<uint8_t>(b);
}
inline constexpr uint8_t operator|(uint8_t a, PrimitiveCapability b) noexcept {
    return a | static_cast<uint8_t>(b);
}
inline constexpr uint8_t operator|(PrimitiveCapability a, uint8_t b) noexcept {
    return static_cast<uint8_t>(a) | b;
}

enum class MaterialType : uint8_t {
    OrganicFiber = 0, // Silk, plant hair, cellulose
    WoodStick = 1,    // Twig, branch
    StonePebble = 2,  // Hard mineral, flint
    ResinAdhesive = 3,// Sticky tree sap, beeswax
    MetalScrap = 4    // Rare conductive/hard scrap
};

inline const char* material_to_string(MaterialType mat) {
    switch (mat) {
        case MaterialType::OrganicFiber: return "OrganicFiber";
        case MaterialType::WoodStick: return "WoodStick";
        case MaterialType::StonePebble: return "StonePebble";
        case MaterialType::ResinAdhesive: return "ResinAdhesive";
        case MaterialType::MetalScrap: return "MetalScrap";
    }
    return "Unknown";
}

struct TechnologyItem {
    uint32_t item_id{0};
    std::string name;
    MaterialType material{MaterialType::WoodStick};
    double mass{0.05};             // kg
    double durability{100.0};      // 0 to 100
    double max_durability{100.0};
    uint8_t capabilities{0};       // Bitmask of PrimitiveCapability
    Vec3 position{0.0, 0.0, 0.0};
    uint32_t connected_to_id{0};   // Connected item constraint (0 = none)
    bool is_active{false};         // Operating state

    [[nodiscard]] bool has_capability(PrimitiveCapability cap) const noexcept {
        return (capabilities & static_cast<uint8_t>(cap)) != 0;
    }

    [[nodiscard]] nlohmann::json to_json() const {
        return {
            {"item_id", item_id},
            {"name", name},
            {"material", material_to_string(material)},
            {"mass", mass},
            {"durability", durability},
            {"capabilities", capabilities},
            {"connected_to", connected_to_id},
            {"active", is_active}
        };
    }
};

struct CraftingRecipe {
    std::string result_name;
    std::vector<MaterialType> required_materials;
    MaterialType result_material;
    uint8_t result_capabilities;
    double result_durability;
};

class ToolSystem {
private:
    std::unordered_map<uint32_t, TechnologyItem> m_items;
    std::vector<CraftingRecipe> m_recipes;
    uint32_t m_next_item_id{1};

public:
    ToolSystem() {
        // Register standard primitive recipes
        // 1. Fiber + Stick = Scythe / Rake (Manipulate | Construct)
        m_recipes.push_back({
            "PrimitiveScraper",
            {MaterialType::WoodStick, MaterialType::OrganicFiber},
            MaterialType::WoodStick,
            static_cast<uint8_t>(PrimitiveCapability::Inspect | PrimitiveCapability::Manipulate),
            80.0
        });
        // 2. Stick + Pebble + Resin = Hammer / Pestle (Manipulate | Combine | Construct)
        m_recipes.push_back({
            "ResinCrusher",
            {MaterialType::WoodStick, MaterialType::StonePebble, MaterialType::ResinAdhesive},
            MaterialType::StonePebble,
            static_cast<uint8_t>(PrimitiveCapability::Manipulate | PrimitiveCapability::Combine | PrimitiveCapability::Construct),
            120.0
        });
        // 3. Stick + MetalScrap = Lever / Conductor (Connect | Operate)
        m_recipes.push_back({
            "MechanismLever",
            {MaterialType::WoodStick, MaterialType::MetalScrap},
            MaterialType::WoodStick,
            static_cast<uint8_t>(PrimitiveCapability::Connect | PrimitiveCapability::Operate),
            150.0
        });
    }

    TechnologyItem* create_primitive(const std::string& name, MaterialType mat, uint8_t caps, double durability = 100.0) {
        uint32_t id = m_next_item_id++;
        TechnologyItem item;
        item.item_id = id;
        item.name = name;
        item.material = mat;
        item.capabilities = caps;
        item.durability = durability;
        item.max_durability = durability;
        auto res = m_items.emplace(id, item);
        return &(res.first->second);
    }

    [[nodiscard]] TechnologyItem* get_item(uint32_t id) {
        auto it = m_items.find(id);
        if (it != m_items.end()) return &it->second;
        return nullptr;
    }

    [[nodiscard]] const TechnologyItem* get_item(uint32_t id) const {
        auto it = m_items.find(id);
        if (it != m_items.end()) return &it->second;
        return nullptr;
    }

    // 1. Inspect capability
    [[nodiscard]] bool inspect_item(uint32_t id, double& out_durability, MaterialType& out_mat) const {
        const auto* item = get_item(id);
        if (!item) return false;
        out_durability = item->durability;
        out_mat = item->material;
        return true;
    }

    // 2. Manipulate capability
    bool manipulate_item(uint32_t id, const Vec3& target_pos) {
        auto* item = get_item(id);
        if (!item || !item->has_capability(PrimitiveCapability::Manipulate)) return false;
        item->position = target_pos;
        item->durability = std::max(0.0, item->durability - 0.1);
        return true;
    }

    // 3. Connect capability
    bool connect_items(uint32_t item_a_id, uint32_t item_b_id) {
        auto* a = get_item(item_a_id);
        auto* b = get_item(item_b_id);
        if (!a || !b) return false;
        if (!a->has_capability(PrimitiveCapability::Connect) && !b->has_capability(PrimitiveCapability::Connect)) {
            return false;
        }
        a->connected_to_id = item_b_id;
        b->connected_to_id = item_a_id;
        return true;
    }

    // 4. Operate capability
    bool operate_item(uint32_t id) {
        auto* item = get_item(id);
        if (!item || !item->has_capability(PrimitiveCapability::Operate)) return false;
        item->is_active = !item->is_active; // Toggle state
        item->durability = std::max(0.0, item->durability - 0.5);
        return true;
    }

    // 5. Combine capability (Crafting via recipe match)
    TechnologyItem* combine_items(const std::vector<uint32_t>& input_ids) {
        if (input_ids.empty()) return nullptr;

        std::vector<MaterialType> input_materials;
        for (uint32_t id : input_ids) {
            auto* item = get_item(id);
            if (!item || item->durability <= 0.0) return nullptr;
            input_materials.push_back(item->material);
        }

        std::sort(input_materials.begin(), input_materials.end());

        for (const auto& recipe : m_recipes) {
            auto req = recipe.required_materials;
            std::sort(req.begin(), req.end());
            if (req == input_materials) {
                // Consume input items
                for (uint32_t id : input_ids) {
                    m_items.erase(id);
                }
                // Produce crafted tool
                return create_primitive(
                    recipe.result_name,
                    recipe.result_material,
                    recipe.result_capabilities,
                    recipe.result_durability
                );
            }
        }
        return nullptr;
    }

    // 6. Construct macro-structure component
    bool construct_component(uint32_t tool_id, const std::string& structure_name) {
        auto* tool = get_item(tool_id);
        if (!tool || !tool->has_capability(PrimitiveCapability::Construct)) return false;
        tool->durability = std::max(0.0, tool->durability - 2.0);
        // Structure constructed
        create_primitive("StructuralWall_" + structure_name, tool->material, static_cast<uint8_t>(PrimitiveCapability::Inspect), 200.0);
        return true;
    }

    [[nodiscard]] size_t item_count() const noexcept { return m_items.size(); }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& [_, it] : m_items) {
            arr.push_back(it.to_json());
        }
        return arr;
    }
};

} // namespace flgod::technology
