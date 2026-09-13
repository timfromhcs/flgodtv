#include <cassert>
#include <iostream>
#include "flgod/llm/god_fly.hpp"
#include "flgod/agents/agent.hpp"

int main() {
    std::cout << "[Test] Running God Fly Unit Tests...\n";

    // 1. Initialize Model Manager and God Fly
    flgod::llm::ModelManager mgr(4096.0, 2048.0);
    bool loaded = mgr.load_model(
        flgod::llm::ModelRole::GodFly,
        "godfly_teacher.gguf",
        flgod::llm::LLMBackend::CPU,
        /*allow_mock_fallback=*/true
    );
    assert(loaded);
    (void)loaded;

    flgod::llm::GodFly god_fly(&mgr);
    assert(god_fly.get_id().raw() != 0);
    assert(god_fly.get_total_lessons_taught() == 0);

    // 2. Create student agent
    flgod::EntityID student_id(flgod::EntityType::Agent, 1, 501);
    flgod::Agent student(student_id);
    assert(student.is_alive());

    // 3. Generate teaching packet across modes
    flgod::llm::TeachingPacket packet_concept;
    bool created_concept = god_fly.create_lesson(
        flgod::llm::TeachingMode::Concept,
        student.id(),
        "nectar_foraging",
        "Student has low energy",
        packet_concept
    );
    assert(created_concept && "Failed to create concept lesson");
    (void)created_concept;
    assert(packet_concept.mode == flgod::llm::TeachingMode::Concept);
    assert(packet_concept.concept_id == "nectar_foraging");
    assert(packet_concept.student_id == student.id());

    // 4. Test delivery of lesson to student
    // Verify initial student state: working memory is empty, no semantic concept, no trust for GodFly
    assert(student.memory().working().items().empty());
    assert(!student.memory().semantic().has_fact("nectar_foraging"));
    assert(student.memory().social().get_trust(god_fly.get_id().raw()) == 0.0);

    bool delivered = god_fly.deliver_lesson(packet_concept, student, 10.0);
    assert(delivered && "Failed to deliver lesson");
    (void)delivered;
    assert(packet_concept.delivered);
    assert(god_fly.get_total_lessons_taught() == 1);

    // 5. CRUCIAL SECTION 54 VERIFICATION:
    // Student receives the lesson into working memory and social trust
    assert(!student.memory().working().items().empty());
    const auto& working_item = student.memory().working().items().back();
    assert(working_item.tag == "lesson_nectar_foraging");
    (void)working_item;

    // Social trust updated
    assert(student.memory().social().get_trust(god_fly.get_id().raw()) > 0.0);

    // Semantic concept entered as unconfirmed hypothesis (confidence = 0.25)
    assert(student.memory().semantic().has_fact("nectar_foraging"));
    double concept_conf = student.memory().semantic().get_fact("nectar_foraging").confidence;
    assert(concept_conf == 0.25 && "Concept must be unconfirmed hypothesis (0.25)!");

    std::cout << "  Student received lesson. Initial concept confidence=" << concept_conf 
              << " (strictly no free skills/rewards)\n";

    // 6. Test other teaching modes (Demonstration, Suggestion, Explanation, Speech)
    flgod::llm::TeachingPacket packet_demo;
    bool created_demo = god_fly.create_lesson(
        flgod::llm::TeachingMode::Demonstration,
        student.id(),
        "evasion_flight",
        "Incoming predator",
        packet_demo
    );
    assert(created_demo);
    (void)created_demo;
    assert(packet_demo.mode == flgod::llm::TeachingMode::Demonstration);

    std::cout << "[Test] God Fly Unit Tests PASSED!\n";
    return 0;
}
