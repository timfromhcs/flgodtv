#include <cassert>
#include <iostream>
#include "flgod/language/social_learning.hpp"

int main() {
    std::cout << "[Test] Running Social Learning & Provenance Unit Tests...\n";

    flgod::language::SocialLearningTracker tracker;

    flgod::EntityID god_fly_id(999);
    flgod::EntityID agent_a(flgod::EntityType::Agent, 1, 1);
    flgod::EntityID agent_b(flgod::EntityType::Agent, 1, 2);
    flgod::EntityID agent_c(flgod::EntityType::Agent, 2, 3);
    flgod::EntityID explorer_d(flgod::EntityType::Agent, 1, 4);

    // 1. Individual experience discovery
    tracker.record_individual_discovery(explorer_d, "rock_shelter", flgod::AgentActionType::Rest, 10.0);
    const auto* prov_d = tracker.get_provenance(explorer_d, "rock_shelter");
    assert(prov_d != nullptr);
    assert(prov_d->origin == flgod::language::KnowledgeOrigin::IndividualExperience);
    assert(prov_d->generation_depth == 0);
    assert(prov_d->transmission_fidelity == 1.0);

    // 2. God Fly pedagogical teaching
    tracker.record_god_fly_lesson(agent_a, god_fly_id, "nectar_harvesting", flgod::AgentActionType::Forage, 12.0);
    const auto* prov_a = tracker.get_provenance(agent_a, "nectar_harvesting");
    assert(prov_a != nullptr);
    assert(prov_a->origin == flgod::language::KnowledgeOrigin::GodFly);
    assert(prov_a->source_agent_id == god_fly_id);
    assert(prov_a->generation_depth == 0);

    // 3. Peer-to-peer social transfer (Agent A -> Agent B via Demonstration)
    bool transmitted_ab = tracker.transmit_peer_to_peer(
        agent_b,
        agent_a,
        "nectar_harvesting",
        flgod::language::TransmissionChannel::Demonstration,
        /*acuity=*/1.0,
        /*dist=*/2.0,
        15.0
    );
    assert(transmitted_ab);
    const auto* prov_b = tracker.get_provenance(agent_b, "nectar_harvesting");
    assert(prov_b != nullptr);
    assert(prov_b->origin == flgod::language::KnowledgeOrigin::SocialTransfer);
    assert(prov_b->source_agent_id == agent_a);
    assert(prov_b->generation_depth == 1);
    assert(prov_b->transmission_fidelity < 1.0 && "Fidelity should decay over transmission hops");

    // 4. Multi-generational cultural inheritance (Agent B -> Agent C via Communication)
    // When generation_depth reaches 2+, it transitions into CulturalInheritance
    bool transmitted_bc = tracker.transmit_peer_to_peer(
        agent_c,
        agent_b,
        "nectar_harvesting",
        flgod::language::TransmissionChannel::Communication,
        /*acuity=*/0.9,
        /*dist=*/1.0,
        25.0
    );
    assert(transmitted_bc);
    const auto* prov_c = tracker.get_provenance(agent_c, "nectar_harvesting");
    assert(prov_c != nullptr);
    assert(prov_c->origin == flgod::language::KnowledgeOrigin::CulturalInheritance);
    assert(prov_c->generation_depth == 2);
    assert(prov_c->source_agent_id == agent_b);

    // 5. Verification of provenance counts
    assert(tracker.count_by_origin(flgod::language::KnowledgeOrigin::IndividualExperience) == 1);
    assert(tracker.count_by_origin(flgod::language::KnowledgeOrigin::GodFly) == 1);
    assert(tracker.count_by_origin(flgod::language::KnowledgeOrigin::SocialTransfer) == 1);
    assert(tracker.count_by_origin(flgod::language::KnowledgeOrigin::CulturalInheritance) == 1);
    assert(tracker.total_transmission_events() == 3);

    // 6. Empirical validation score updates
    tracker.update_empirical_score(agent_c, "nectar_harvesting", 1.0);
    assert(tracker.get_provenance(agent_c, "nectar_harvesting")->empirical_validation_score > 0.0);

    std::cout << "[Test] Social Learning & Provenance Unit Tests PASSED!\n";
    return 0;
}
