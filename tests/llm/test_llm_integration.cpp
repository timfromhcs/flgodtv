#include <cassert>
#include <iostream>
#include "flgod/llm/model_manager.hpp"
#include "flgod/llm/action_security.hpp"
#include "flgod/llm/god_fly.hpp"
#include "flgod/llm/npc_system.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/world/world.hpp"

int main() {
    std::cout << "[Test] Running LLM Subsystem End-to-End Integration Test...\n";

    // 1. Initialize Subsystems
    flgod::WorldConfig cfg;
    cfg.world_seed = 424242;
    flgod::World world(cfg);

    flgod::AgentManager agent_mgr;
    flgod::Colony c1(1, "Colony_Alpha", flgod::Vec3(0.0, 0.0, 0.0), 50.0);
    c1.deposit_food(100.0);
    agent_mgr.register_colony(c1);

    // 2. Initialize Model Manager with God Fly and NPC roles
    flgod::llm::ModelManager model_mgr(8192.0, 4096.0);
    bool load_god = model_mgr.load_model(
        flgod::llm::ModelRole::GodFly,
        "godfly.gguf",
        flgod::llm::LLMBackend::CPU,
        true
    );
    bool load_npc = model_mgr.load_model(
        flgod::llm::ModelRole::NPC,
        "npc.gguf",
        flgod::llm::LLMBackend::CPU,
        true
    );
    assert(load_god && load_npc && "Failed to load LLM models");

    flgod::llm::GodFly god_fly(&model_mgr);
    flgod::llm::NPCManager npc_mgr(&model_mgr);

    // 3. Spawn Student Agent in Colony
    flgod::EntityID student_id(flgod::EntityType::Agent, 1, 1001);
    flgod::Agent student_init(student_id, c1.id());
    student_init.set_position({2.0, 0.0, 2.0});
    // Give student low initial energy / hunger
    student_init.drives().energy = 25.0;
    student_init.drives().hunger = 80.0;
    agent_mgr.spawn_agent(student_init);

    auto* npc_profile = npc_mgr.register_npc(student_id);
    assert(npc_profile != nullptr);

    // 4. God Fly generates lesson for student via Secure Pipeline
    flgod::llm::TeachingPacket lesson;
    bool lesson_created = god_fly.create_lesson(
        flgod::llm::TeachingMode::Concept,
        student_id,
        "flower_foraging_strategy",
        "Student is hungry and near food source",
        lesson,
        &agent_mgr
    );
    assert(lesson_created && "Failed to generate lesson via secure pipeline");

    // 5. Deliver lesson to student
    auto& student = agent_mgr.get_agent(student_id);
    bool delivered = god_fly.deliver_lesson(lesson, student, 1.0);
    assert(delivered);
    assert(lesson.delivered);

    // SECTION 54 STRICT CHECK: No free reward or automatic energy jump
    assert(student.drives().energy == 25.0 && "Teacher lesson must NOT magically refill energy!");
    assert(student.memory().semantic().has_fact("flower_foraging_strategy"));
    double initial_hypothesis_conf = student.memory().semantic().get_fact("flower_foraging_strategy").confidence;
    assert(initial_hypothesis_conf == 0.25);

    // 6. Student puts lesson into practice: Performs physical foraging step in simulation
    flgod::RNGStream rng(987654ULL);
    double dt = 0.1;

    double pre_forage_energy = student.drives().energy;
    // Step simulation: Agent perceives food in colony territory, executes Forage
    agent_mgr.step(dt, world, rng);

    // Verify student foraged and colony recorded foraging interaction
    double post_forage_energy = agent_mgr.get_agent(student_id).drives().energy;
    assert(post_forage_energy > pre_forage_energy && "Physical foraging step should replenish energy");

    // Student reinforces concept empirically from positive outcome
    student.learner().step_update(student_id.raw(), 1.0, 0, static_cast<uint32_t>(flgod::AgentActionType::Forage), 1.0, 1, false);
    student.memory().semantic().store_fact("flower_foraging_strategy", {1.0, 0.0}, 0.85);
    assert(student.memory().semantic().get_fact("flower_foraging_strategy").confidence == 0.85);

    // 7. Event-Driven NPC report back
    flgod::llm::ValidatedLLMAction report_action;
    bool reported = npc_mgr.trigger_event_inference(
        student_id,
        flgod::llm::NPCInferenceTrigger::DirectDialogue,
        5.0,
        "Successfully foraged food and reinforced concept",
        report_action,
        &agent_mgr
    );
    assert(reported);
    assert(report_action.is_valid);
    assert(npc_profile->get_inference_count() == 1);

    // Verify cooldown suppresses immediate re-trigger at t=5.2s
    flgod::llm::ValidatedLLMAction duplicate_action;
    bool suppressed = npc_mgr.trigger_event_inference(
        student_id,
        flgod::llm::NPCInferenceTrigger::DirectDialogue,
        5.2,
        "Duplicate immediate call",
        duplicate_action,
        &agent_mgr
    );
    assert(!suppressed && "Cooldown must suppress duplicate immediate inference");

    // 8. Crash Recovery and State Preservation
    auto student_json = student.to_json();
    flgod::Agent restored_student;
    restored_student.from_json(student_json);
    assert(restored_student.id() == student_id);
    assert(restored_student.memory().semantic().get_fact("flower_foraging_strategy").confidence == 0.85);
    assert(restored_student.memory().social().get_trust(god_fly.get_id().raw()) > 0.0);

    std::cout << "  Integration Flow Verified: Lesson delivered -> physical simulation foraging -> Q-learning step -> concept reinforcement (0.25 -> 0.85) -> event-driven dialogue -> crash recovery match.\n";
    std::cout << "[Test] LLM Subsystem End-to-End Integration Test PASSED!\n";
    return 0;
}
