#include <cassert>
#include <iostream>
#include "flgod/language/signal_symbol.hpp"
#include "flgod/language/vocabulary.hpp"
#include "flgod/language/social_learning.hpp"
#include "flgod/agents/agent_manager.hpp"
#include "flgod/world/world.hpp"

int main() {
    std::cout << "[Test] Running Language & Cultural Transmission Integration Test...\n";

    // 1. Initialize Subsystems
    flgod::language::SignalSymbolSystem symbol_sys;
    flgod::language::VocabularySystem vocab_sys;
    flgod::language::SocialLearningTracker tracker;

    flgod::EntityID god_fly_id(999999);

    // 2. Colony Population Setup across 3 generations
    flgod::EntityID gen1_lead(flgod::EntityType::Agent, 1, 101);
    flgod::EntityID gen1_peer(flgod::EntityType::Agent, 1, 102);
    flgod::EntityID gen2_student(flgod::EntityType::Agent, 2, 201);
    flgod::EntityID gen3_offspring(flgod::EntityType::Agent, 3, 301);

    // 3. Phase 1: God Fly Pedagogical Injection
    tracker.record_god_fly_lesson(
        gen1_lead,
        god_fly_id,
        "high_yield_foraging",
        flgod::AgentActionType::Forage,
        1.0
    );
    assert(tracker.get_provenance(gen1_lead, "high_yield_foraging")->origin == flgod::language::KnowledgeOrigin::GodFly);

    // 4. Phase 2: Horizontal Social Transfer (Gen 1 Lead -> Gen 1 Peer via Demonstration)
    bool peer_ok = tracker.transmit_peer_to_peer(
        gen1_peer,
        gen1_lead,
        "high_yield_foraging",
        flgod::language::TransmissionChannel::Demonstration,
        1.0, // Acuity
        1.5, // Distance
        2.0  // Timestamp
    );
    assert(peer_ok);
    assert(tracker.get_provenance(gen1_peer, "high_yield_foraging")->origin == flgod::language::KnowledgeOrigin::SocialTransfer);
    assert(tracker.get_provenance(gen1_peer, "high_yield_foraging")->generation_depth == 1);

    // 5. Phase 3: Vertical Cultural Transmission (Gen 1 Peer -> Gen 2 Student via Communication)
    // Gen depth reaches 2 => becomes CulturalInheritance
    bool gen2_ok = tracker.transmit_peer_to_peer(
        gen2_student,
        gen1_peer,
        "high_yield_foraging",
        flgod::language::TransmissionChannel::Communication,
        0.95,
        1.0,
        10.0
    );
    assert(gen2_ok);
    assert(tracker.get_provenance(gen2_student, "high_yield_foraging")->origin == flgod::language::KnowledgeOrigin::CulturalInheritance);
    assert(tracker.get_provenance(gen2_student, "high_yield_foraging")->generation_depth == 2);

    // 6. Phase 4: Third-Generation Tradition Transmission (Gen 2 Student -> Gen 3 Offspring via Imitation)
    bool gen3_ok = tracker.transmit_peer_to_peer(
        gen3_offspring,
        gen2_student,
        "high_yield_foraging",
        flgod::language::TransmissionChannel::Imitation,
        0.90,
        2.0,
        20.0
    );
    assert(gen3_ok);
    assert(tracker.get_provenance(gen3_offspring, "high_yield_foraging")->origin == flgod::language::KnowledgeOrigin::CulturalInheritance);
    assert(tracker.get_provenance(gen3_offspring, "high_yield_foraging")->generation_depth == 3);

    // 7. Symbolic Communication and Vocabulary Consensus Formation
    // Colony agents repeatedly broadcast standardized syntax: [ACT_FORAGE, OBJ_NECTAR, QUAL_ABUNDANT]
    for (int step = 0; step < 50; ++step) {
        flgod::language::SymbolicUtterance utt;
        utt.sequence = {101, 201, 301}; // ACT_FORAGE, OBJ_NECTAR, QUAL_ABUNDANT
        utt.speaker_id = gen2_student;
        utt.timestamp = 10.0 + step * 0.1;
        vocab_sys.record_utterance(utt);

        // Grounding reinforcement for symbol 1 (SYM_FOOD_NECTAR)
        symbol_sys.record_interaction_outcome(1, true, 0.05);
    }

    // High transition probability consensus formed
    double prob_nectar_after_forage = vocab_sys.get_transition_probability(101, 201);
    assert(prob_nectar_after_forage > 0.95 && "Colony vocabulary consensus should converge above 0.95");
    assert(symbol_sys.get_symbol(1)->confidence > 0.95);

    // 8. State Serialization & Recovery
    auto tracker_json = tracker.to_json();
    assert(tracker_json.contains("transmissions"));
    assert(tracker_json.contains("knowledge"));

    auto vocab_json = vocab_sys.to_json();
    assert(vocab_json.contains("words"));
    assert(vocab_json.contains("bigrams"));

    std::cout << "  Cultural Transmission Verified: God Fly (0) -> Social Peer (1) -> Cultural Student (2) -> Cultural Tradition (3).\n";
    std::cout << "  Vocabulary Consensus P(OBJ_NECTAR | ACT_FORAGE) = " << prob_nectar_after_forage 
              << ", Symbol Grounding Confidence = " << symbol_sys.get_symbol(1)->confidence << "\n";
    std::cout << "[Test] Language & Cultural Transmission Integration Test PASSED!\n";
    return 0;
}
