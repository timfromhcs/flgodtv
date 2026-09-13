#include <cassert>
#include <iostream>
#include "flgod/language/vocabulary.hpp"

int main() {
    std::cout << "[Test] Running Vocabulary & Sequence Patterns Unit Tests...\n";

    flgod::language::VocabularySystem vocab;
    assert(vocab.word_count() >= 10);

    // 1. Check foundational words
    assert(vocab.has_word(101)); // ACT_FORAGE
    assert(vocab.get_word(101)->category == flgod::language::SymbolCategory::Action);
    assert(vocab.has_word(201)); // OBJ_NECTAR
    assert(vocab.get_word(201)->category == flgod::language::SymbolCategory::Object);

    // 2. Syntax validation
    // Valid 2-word: [ACT_FORAGE, OBJ_NECTAR]
    assert(vocab.validate_syntax({101, 201}));
    // Valid 3-word: [ACT_FORAGE, OBJ_NECTAR, QUAL_ABUNDANT]
    assert(vocab.validate_syntax({101, 201, 301}));
    // Valid 3-word with direction: [ACT_EVADE, OBJ_PREDATOR, DIR_NORTH]
    assert(vocab.validate_syntax({102, 203, 401}));

    // Invalid syntax: [OBJ_NECTAR, ACT_FORAGE] (Object before Action)
    assert(!vocab.validate_syntax({201, 101}));
    // Invalid syntax: single word
    assert(!vocab.validate_syntax({101}));
    // Invalid syntax: too long
    assert(!vocab.validate_syntax({101, 201, 301, 401, 402}));

    // 3. Sequence patterns and bigram transition probability learning
    flgod::language::SymbolicUtterance utt1;
    utt1.sequence = {101, 201, 301}; // FORAGE NECTAR ABUNDANT
    utt1.speaker_id = flgod::EntityID(flgod::EntityType::Agent, 1, 10);
    utt1.timestamp = 1.0;

    flgod::language::SymbolicUtterance utt2;
    utt2.sequence = {101, 201, 302}; // FORAGE NECTAR DEPLETED
    utt2.speaker_id = flgod::EntityID(flgod::EntityType::Agent, 1, 11);
    utt2.timestamp = 2.0;

    flgod::language::SymbolicUtterance utt3;
    utt3.sequence = {101, 202};      // FORAGE WATER
    utt3.speaker_id = flgod::EntityID(flgod::EntityType::Agent, 1, 12);
    utt3.timestamp = 3.0;

    vocab.record_utterance(utt1);
    vocab.record_utterance(utt2);
    vocab.record_utterance(utt3);

    assert(vocab.total_utterances() == 3);
    assert(vocab.get_word(101)->usage_frequency == 3);
    assert(vocab.get_word(201)->usage_frequency == 2);

    // Probability P(OBJ_NECTAR | ACT_FORAGE) = 2 / 3 = 0.6667
    double p_nectar_forage = vocab.get_transition_probability(101, 201);
    assert(std::abs(p_nectar_forage - (2.0 / 3.0)) < 1e-5);

    // Probability P(OBJ_WATER | ACT_FORAGE) = 1 / 3 = 0.3333
    double p_water_forage = vocab.get_transition_probability(101, 202);
    assert(std::abs(p_water_forage - (1.0 / 3.0)) < 1e-5);

    // 4. Deterministic hashing & serialization
    uint64_t hash1 = vocab.compute_hash();
    auto json_data = vocab.to_json();
    assert(!json_data.empty());
    assert(hash1 != 0);

    std::cout << "[Test] Vocabulary & Sequence Patterns Unit Tests PASSED!\n";
    return 0;
}
