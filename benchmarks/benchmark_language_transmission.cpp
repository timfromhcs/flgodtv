#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "flgod/language/signal_symbol.hpp"
#include "flgod/language/vocabulary.hpp"
#include "flgod/language/social_learning.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << "  FLGODTV LANGUAGE & SOCIAL LEARNING BENCHMARK\n";
    std::cout << "========================================================\n";

    flgod::language::SignalSymbolSystem symbol_sys;
    flgod::language::VocabularySystem vocab_sys;
    flgod::language::SocialLearningTracker tracker;

    // 1. Vocabulary Utterance & Sequence Learning Benchmark
    const int UTTERANCE_COUNT = 100000;
    auto t_utt_start = std::chrono::steady_clock::now();

    for (int i = 0; i < UTTERANCE_COUNT; ++i) {
        flgod::language::SymbolicUtterance utt;
        utt.sequence = {101, 201, 301};
        utt.speaker_id = flgod::EntityID(flgod::EntityType::Agent, 1, i % 100 + 1);
        utt.timestamp = i * 0.01;
        vocab_sys.record_utterance(utt);
    }

    auto t_utt_end = std::chrono::steady_clock::now();
    double utt_time_ms = std::chrono::duration<double, std::milli>(t_utt_end - t_utt_start).count();
    double utterances_per_sec = (static_cast<double>(UTTERANCE_COUNT) / (utt_time_ms / 1000.0));

    std::cout << "Utterance Processing: " << UTTERANCE_COUNT << " utterances in " << utt_time_ms << " ms\n";
    std::cout << "Utterance Throughput:  " << utterances_per_sec << " utterances/sec\n";

    // 2. Social Transmission & Provenance Tracking Benchmark
    const int TRANSMISSION_COUNT = 50000;
    auto t_trans_start = std::chrono::steady_clock::now();

    flgod::EntityID root_id(1);
    tracker.record_god_fly_lesson(root_id, flgod::EntityID(999), "forage_technique", flgod::AgentActionType::Forage, 0.0);

    for (int i = 1; i <= TRANSMISSION_COUNT; ++i) {
        flgod::EntityID parent_id(static_cast<uint64_t>(i));
        flgod::EntityID child_id(static_cast<uint64_t>(i + 1));
        tracker.transmit_peer_to_peer(
            child_id,
            parent_id,
            "forage_technique",
            flgod::language::TransmissionChannel::Communication,
            0.99,
            0.5,
            i * 0.1
        );
    }

    auto t_trans_end = std::chrono::steady_clock::now();
    double trans_time_ms = std::chrono::duration<double, std::milli>(t_trans_end - t_trans_start).count();
    double transmissions_per_sec = (static_cast<double>(TRANSMISSION_COUNT) / (trans_time_ms / 1000.0));

    std::cout << "Social Transmissions: " << TRANSMISSION_COUNT << " hops in " << trans_time_ms << " ms\n";
    std::cout << "Transmission Speed:   " << transmissions_per_sec << " transmissions/sec\n";

    // 3. Save Evidence JSON
    nlohmann::json bench_json;
    bench_json["utterances_tested"] = UTTERANCE_COUNT;
    bench_json["utterance_time_ms"] = utt_time_ms;
    bench_json["utterances_per_second"] = utterances_per_sec;
    bench_json["transmissions_tested"] = TRANSMISSION_COUNT;
    bench_json["transmission_time_ms"] = trans_time_ms;
    bench_json["transmissions_per_second"] = transmissions_per_sec;
    bench_json["vocabulary_word_count"] = vocab_sys.word_count();
    bench_json["symbol_count"] = symbol_sys.symbol_count();

    std::filesystem::create_directories("evidence/windows");
    std::string out_path = "evidence/windows/language_benchmark.json";
    std::ofstream out(out_path);
    out << bench_json.dump(2) << std::endl;
    out.close();

    std::cout << "Saved benchmark evidence to: " << out_path << "\n";
    std::cout << "========================================================\n";
    return 0;
}
