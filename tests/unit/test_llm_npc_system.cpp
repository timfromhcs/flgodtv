#include <cassert>
#include <iostream>
#include "flgod/llm/npc_system.hpp"

int main() {
    std::cout << "[Test] Running NPC System Unit Tests...\n";

    // 1. Initialize Model Manager and NPC Manager
    flgod::llm::ModelManager mgr(4096.0, 2048.0);
    bool loaded = mgr.load_model(
        flgod::llm::ModelRole::NPC,
        "npc_agent.gguf",
        flgod::llm::LLMBackend::CPU,
        /*allow_mock_fallback=*/true
    );
    assert(loaded);
    (void)loaded;

    flgod::llm::NPCManager npc_mgr(&mgr);

    // 2. Register NPC
    flgod::EntityID npc_id(flgod::EntityType::Agent, 1, 701);
    flgod::llm::NPCAgent* npc = npc_mgr.register_npc(npc_id);
    assert(npc != nullptr);
    assert(npc_mgr.count() == 1);

    // 3. Configure Personality, Goals, Relationships, Knowledge
    npc->get_personality().curiosity = 0.85f;
    npc->get_personality().sociability = 0.70f;
    assert(npc->get_personality().curiosity == 0.85f);

    npc->add_goal("FindWater", 1.5f, {15.0, 0.0, 10.0});
    assert(npc->get_goals().size() == 1);

    flgod::EntityID peer_id(flgod::EntityType::Agent, 1, 702);
    npc->set_relationship(peer_id, 0.6f);
    assert(npc->get_trust(peer_id) == 0.6f);

    npc->set_knowledge("hydrated_moss", 0.4f);
    assert(npc->get_knowledge("hydrated_moss") == 0.4f);

    // 4. SECTION 55 VERIFICATION: Event-Driven Inference & Anti-Polling
    // First trigger at t = 1.0s should succeed
    flgod::llm::ValidatedLLMAction action1;
    bool trig1 = npc_mgr.trigger_event_inference(
        npc_id,
        flgod::llm::NPCInferenceTrigger::CriticalResource,
        1.0,
        "Hydration below 15%",
        action1
    );
    assert(trig1 && "First event inference should trigger");
    (void)trig1;
    assert(action1.is_valid);
    assert(npc->get_inference_count() == 1);
    assert(!npc->get_history().empty());

    // Second trigger immediately at t = 1.5s (within 3.0s cooldown) MUST BE REJECTED
    flgod::llm::ValidatedLLMAction action_suppressed;
    bool trig_busy = npc_mgr.trigger_event_inference(
        npc_id,
        flgod::llm::NPCInferenceTrigger::CriticalResource,
        1.5,
        "Hydration still low",
        action_suppressed
    );
    assert(!trig_busy && "Busy per-tick polling inference MUST be suppressed by cooldown!");
    (void)trig_busy;
    assert(npc->get_inference_count() == 1 && "Inference count must not increment when suppressed");

    // Third trigger at t = 5.0s (after cooldown passes) should succeed
    flgod::llm::ValidatedLLMAction action2;
    bool trig2 = npc_mgr.trigger_event_inference(
        npc_id,
        flgod::llm::NPCInferenceTrigger::NovelDiscovery,
        5.0,
        "Discovered moisture seep",
        action2
    );
    assert(trig2 && "Inference should succeed after cooldown expires");
    (void)trig2;
    assert(npc->get_inference_count() == 2);
    assert(npc->get_history().size() == 2);

    std::cout << "  Event-driven cooldown verified: 1st=PASS, 2nd (0.5s)=SUPPRESSED, 3rd (4.0s)=PASS.\n";
    std::cout << "[Test] NPC System Unit Tests PASSED!\n";
    return 0;
}
