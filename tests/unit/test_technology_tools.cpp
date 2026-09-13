#include <cassert>
#include <iostream>
#include "flgod/technology/tool_system.hpp"

int main() {
    std::cout << "[Test] Running Technology Tool System Unit Tests...\n";

    flgod::technology::ToolSystem tools;

    // 1. Primitive creation
    auto* stick = tools.create_primitive(
        "Twig",
        flgod::technology::MaterialType::WoodStick,
        static_cast<uint8_t>(flgod::technology::PrimitiveCapability::Manipulate | flgod::technology::PrimitiveCapability::Connect),
        100.0
    );
    assert(stick != nullptr);
    uint32_t stick_id = stick->item_id;

    auto* fiber = tools.create_primitive(
        "SilkFiber",
        flgod::technology::MaterialType::OrganicFiber,
        static_cast<uint8_t>(flgod::technology::PrimitiveCapability::Manipulate),
        50.0
    );
    assert(fiber != nullptr);
    uint32_t fiber_id = fiber->item_id;

    // 2. Inspect capability
    double dura = 0.0;
    flgod::technology::MaterialType mat;
    bool inspected = tools.inspect_item(stick_id, dura, mat);
    assert(inspected);
    assert(dura == 100.0);
    assert(mat == flgod::technology::MaterialType::WoodStick);

    // 3. Manipulate capability
    bool manipulated = tools.manipulate_item(stick_id, {5.0, 1.0, 5.0});
    assert(manipulated);
    assert(stick->position.x == 5.0);
    assert(stick->durability < 100.0 && "Manipulation should incur minor wear");

    // 4. Connect capability
    auto* metal = tools.create_primitive(
        "Wire",
        flgod::technology::MaterialType::MetalScrap,
        static_cast<uint8_t>(flgod::technology::PrimitiveCapability::Connect | flgod::technology::PrimitiveCapability::Operate)
    );
    assert(metal != nullptr);
    bool connected = tools.connect_items(stick_id, metal->item_id);
    assert(connected);
    assert(stick->connected_to_id == metal->item_id);
    assert(metal->connected_to_id == stick_id);

    // 5. Operate capability
    assert(!metal->is_active);
    bool operated = tools.operate_item(metal->item_id);
    assert(operated);
    assert(metal->is_active);
    tools.operate_item(metal->item_id); // Toggle back
    assert(!metal->is_active);

    // 6. Combine capability (Crafting: Fiber + Stick -> PrimitiveScraper)
    auto* crafted_tool = tools.combine_items({stick_id, fiber_id});
    assert(crafted_tool != nullptr);
    assert(crafted_tool->name == "PrimitiveScraper");
    assert(tools.get_item(stick_id) == nullptr && "Consumed input stick should be removed");
    assert(tools.get_item(fiber_id) == nullptr && "Consumed input fiber should be removed");

    // 7. Construct capability (Create structural component)
    auto* builder_tool = tools.create_primitive(
        "BuilderHammer",
        flgod::technology::MaterialType::StonePebble,
        static_cast<uint8_t>(flgod::technology::PrimitiveCapability::Construct)
    );
    assert(builder_tool != nullptr);
    size_t prev_count = tools.item_count();
    bool constructed = tools.construct_component(builder_tool->item_id, "NorthNest");
    assert(constructed);
    assert(tools.item_count() == prev_count + 1);

    std::cout << "[Test] Technology Tool System Unit Tests PASSED!\n";
    return 0;
}
