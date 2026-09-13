#include "flgod/core/rng.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>

int main() {
    std::cout << "[TEST] Running test_core_rng..." << std::endl;

    // 1. Determinism test
    const uint64_t seed = 4294967296ULL + 12345ULL;
    flgod::RNGStream rng1(seed);
    flgod::RNGStream rng2(seed);

    for (int i = 0; i < 1000; ++i) {
        uint64_t v1 = rng1.next_u64();
        uint64_t v2 = rng2.next_u64();
        if (v1 != v2) {
            std::cerr << "FAILED: Deterministic RNG stream divergence at step " << i << std::endl;
            return 1;
        }
    }

    if (rng1.compute_hash() != rng2.compute_hash()) {
        std::cerr << "FAILED: Hash mismatch for identical RNG states!" << std::endl;
        return 1;
    }

    // 2. Different seeds yield different outputs
    flgod::RNGStream rng3(seed + 1);
    if (rng1.compute_hash() == rng3.compute_hash()) {
        std::cerr << "FAILED: Different seeds produced identical state!" << std::endl;
        return 1;
    }

    // 3. Uniform integer bounds test
    const int64_t min_val = -50;
    const int64_t max_val = 50;
    for (int i = 0; i < 5000; ++i) {
        int64_t val = rng1.uniform_int(min_val, max_val);
        if (val < min_val || val > max_val) {
            std::cerr << "FAILED: uniform_int out of bounds: " << val << " not in [" << min_val << ", " << max_val << "]" << std::endl;
            return 1;
        }
    }

    // 4. Uniform real bounds test
    for (int i = 0; i < 5000; ++i) {
        double d = rng1.next_double();
        if (d < 0.0 || d >= 1.0) {
            std::cerr << "FAILED: next_double out of [0, 1): " << d << std::endl;
            return 1;
        }
    }

    // 5. Gaussian distribution test
    const double mean = 10.0;
    const double stddev = 2.5;
    const int n_samples = 20000;
    double sum = 0.0;
    for (int i = 0; i < n_samples; ++i) {
        double g = rng1.gaussian(mean, stddev);
        sum += g;
    }
    double sample_mean = sum / n_samples;
    if (std::abs(sample_mean - mean) > 0.1) {
        std::cerr << "FAILED: Gaussian mean deviated too far: expected " << mean << ", got " << sample_mean << std::endl;
        return 1;
    }

    // 6. Independent streams in DeterministicRNG
    flgod::RNGSeeds seeds{
        .world_seed = 101,
        .weather_seed = 102,
        .physics_seed = 103,
        .agent_seed = 104,
        .genome_seed = 105,
        .event_seed = 106,
        .learning_seed = 107,
        .render_seed = 108
    };
    flgod::DeterministicRNG det_rngA(seeds);
    flgod::DeterministicRNG det_rngB(seeds);

    if (det_rngA.compute_hash() != det_rngB.compute_hash()) {
        std::cerr << "FAILED: DeterministicRNG hash mismatch on same seeds!" << std::endl;
        return 1;
    }

    // Advance world stream on A
    det_rngA.world().next_u64();
    if (det_rngA.compute_hash() == det_rngB.compute_hash()) {
        std::cerr << "FAILED: Advancing stream did not change DeterministicRNG hash!" << std::endl;
        return 1;
    }

    std::cout << "[PASS] test_core_rng passed successfully." << std::endl;
    return 0;
}
