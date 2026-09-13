#include <cassert>
#include <iostream>
#include "flgod/language/signal_symbol.hpp"

int main() {
    std::cout << "[Test] Running Language Signal & Symbol Unit Tests...\n";

    flgod::language::SignalSymbolSystem sys;
    assert(sys.symbol_count() >= 6);

    // 1. Foundational symbol lookup
    assert(sys.has_symbol(1));
    const auto* sym1 = sys.get_symbol(1);
    assert(sym1 != nullptr);
    assert(sym1->token_str == "SYM_FOOD_NECTAR");
    assert(sym1->referent == flgod::language::SemanticReferent::FoodSource);
    assert(sym1->confidence == 0.9);

    uint32_t danger_sym = sys.get_symbol_for_referent(flgod::language::SemanticReferent::DangerPredator);
    assert(danger_sym == 3);

    // 2. Physical signal instantiation
    flgod::language::PhysicalSignal sig;
    sig.channel = flgod::language::SignalChannel::AcousticWingBuzz;
    sig.emitter_id = flgod::EntityID(flgod::EntityType::Agent, 1, 101);
    sig.frequency_hz = 240.0;
    sig.intensity = 0.85;
    sig.discrete_symbol_id = 1;
    sig.bearing_degrees = 45.0;
    assert(sig.channel == flgod::language::SignalChannel::AcousticWingBuzz);

    // 3. Empirical interaction outcome learning (grounding reinforcement & penalty)
    double initial_conf = sym1->confidence;
    // Successful interaction: confidence increases
    sys.record_interaction_outcome(1, true, 0.2);
    assert(sys.get_symbol(1)->confidence > initial_conf);
    assert(sys.get_symbol(1)->usage_count == 1);
    assert(sys.get_symbol(1)->success_count == 1);

    // Unsuccessful interaction: confidence drops
    double boosted_conf = sys.get_symbol(1)->confidence;
    sys.record_interaction_outcome(1, false, 0.2);
    assert(sys.get_symbol(1)->confidence < boosted_conf);
    assert(sys.get_symbol(1)->usage_count == 2);

    // 4. Custom symbol registration
    sys.register_symbol(10, "SYM_PHEROMONE_TRAIL", flgod::language::SemanticReferent::FollowMe, 0.6);
    assert(sys.has_symbol(10));
    assert(sys.get_symbol(10)->token_str == "SYM_PHEROMONE_TRAIL");

    std::cout << "[Test] Language Signal & Symbol Unit Tests PASSED!\n";
    return 0;
}
