#include <cassert>
#include <iostream>
#include "flgod/llm/model_manager.hpp"

int main() {
    std::cout << "[Test] Running Model Manager Unit Tests...\n";

    // Set a strict budget: 4096 MB RAM, 2048 MB VRAM
    flgod::llm::ModelManager mgr(4096.0, 2048.0);
    assert(mgr.get_budget().max_ram_mb == 4096.0);
    assert(mgr.get_budget().max_vram_mb == 2048.0);
    assert(mgr.get_budget().current_ram_mb == 0.0);

    // 1. Initial health check
    auto health_initial = mgr.check_health(flgod::llm::ModelRole::GodFly);
    assert(!health_initial.is_loaded);
    assert(!health_initial.is_healthy);

    // 2. Load God Fly model (Instruct) via CPU backend with fallback mock
    bool loaded_god = mgr.load_model(
        flgod::llm::ModelRole::GodFly,
        "nonexistent_godfly.gguf",
        flgod::llm::LLMBackend::CPU,
        /*allow_mock_fallback=*/true
    );
    assert(loaded_god && "Failed to load GodFly model");
    (void)loaded_god;
    assert(mgr.is_loaded(flgod::llm::ModelRole::GodFly));

    auto health_god = mgr.check_health(flgod::llm::ModelRole::GodFly);
    assert(health_god.is_loaded);
    assert(health_god.is_healthy);
    assert(mgr.get_budget().current_ram_mb > 0.0);

    // 3. Load NPC model (Economical) via Vulkan backend
    bool loaded_npc = mgr.load_model(
        flgod::llm::ModelRole::NPC,
        "nonexistent_npc.gguf",
        flgod::llm::LLMBackend::Vulkan,
        /*allow_mock_fallback=*/true
    );
    assert(loaded_npc && "Failed to load NPC model");
    (void)loaded_npc;
    assert(mgr.is_loaded(flgod::llm::ModelRole::NPC));
    assert(mgr.get_budget().current_vram_mb > 0.0);

    // 4. Generate structured responses and verify performance metrics
    std::string response_god = mgr.generate_response(flgod::llm::ModelRole::GodFly, "Teach foraging to student");
    assert(!response_god.empty());

    const auto* metrics_god = mgr.get_metrics(flgod::llm::ModelRole::GodFly);
    assert(metrics_god != nullptr);
    assert(metrics_god->total_inference_calls == 1);
    assert(metrics_god->total_tokens_generated > 0);
    assert(metrics_god->tokens_per_second > 0.0);
    assert(metrics_god->ram_usage_mb > 0.0);

    std::cout << "  GodFly Metrics: TTFT=" << metrics_god->time_to_first_token_ms 
              << "ms, tok/s=" << metrics_god->tokens_per_second 
              << ", RAM=" << metrics_god->ram_usage_mb << "MB\n";

    // 5. Model unloading and memory deallocation
    bool unloaded_npc = mgr.unload_model(flgod::llm::ModelRole::NPC);
    assert(unloaded_npc && "Failed to unload NPC model");
    (void)unloaded_npc;
    assert(!mgr.is_loaded(flgod::llm::ModelRole::NPC));
    assert(mgr.get_budget().current_vram_mb == 0.0); // VRAM restored

    // 6. Memory budget overflow prevention
    flgod::llm::ModelManager tight_mgr(500.0, 100.0); // Very small budget
    bool tight_load = tight_mgr.load_model(
        flgod::llm::ModelRole::GodFly,
        "godfly.gguf",
        flgod::llm::LLMBackend::CPU,
        true
    );
    // 1950MB model will not fit in 500MB budget!
    assert(!tight_load && "Oversized model load should be rejected by memory budget");
    (void)tight_load;

    std::cout << "[Test] Model Manager Unit Tests PASSED!\n";
    return 0;
}
