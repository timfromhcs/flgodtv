#include "flgod/world/noise.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_world_noise..." << std::endl;

    flgod::DeterministicNoise noise1(12345ULL);
    flgod::DeterministicNoise noise2(12345ULL);
    flgod::DeterministicNoise noise3(99999ULL);

    // 1. Determinism test
    for (double y = -5.0; y <= 5.0; y += 0.5) {
        for (double x = -5.0; x <= 5.0; x += 0.5) {
            double v1 = noise1.noise2d(x, y);
            double v2 = noise2.noise2d(x, y);
            if (v1 != v2) {
                std::cerr << "FAILED: Determinism failure in noise2d at (" << x << ", " << y << ")" << std::endl;
                return 1;
            }

            double f1 = noise1.fbm2d(x, y, 4);
            double f2 = noise2.fbm2d(x, y, 4);
            if (f1 != f2) {
                std::cerr << "FAILED: Determinism failure in fbm2d at (" << x << ", " << y << ")" << std::endl;
                return 1;
            }

            double r1 = noise1.ridged2d(x, y, 4);
            double r2 = noise2.ridged2d(x, y, 4);
            if (r1 != r2) {
                std::cerr << "FAILED: Determinism failure in ridged2d at (" << x << ", " << y << ")" << std::endl;
                return 1;
            }
        }
    }

    // 2. Different seeds produce different outputs
    bool different = false;
    for (double x = 0.1; x < 2.0; x += 0.3) {
        if (noise1.noise2d(x, 1.0) != noise3.noise2d(x, 1.0)) {
            different = true;
            break;
        }
    }
    if (!different) {
        std::cerr << "FAILED: Different seeds produced identical noise values!" << std::endl;
        return 1;
    }

    // 3. Continuity test (no huge jumps over small delta)
    for (double x = 0.0; x < 10.0; x += 0.1) {
        double v1 = noise1.noise2d(x, 0.0);
        double v2 = noise1.noise2d(x + 0.001, 0.0);
        if (std::abs(v2 - v1) > 0.05) {
            std::cerr << "FAILED: Discontinuity in noise2d at x=" << x << std::endl;
            return 1;
        }
    }

    // 4. Bounds check
    for (double y = 0.0; y < 20.0; y += 0.5) {
        for (double x = 0.0; x < 20.0; x += 0.5) {
            double v = noise1.noise2d(x, y);
            if (v < -1.1 || v > 1.1) {
                std::cerr << "FAILED: noise2d out of reasonable range [-1.1, 1.1]: " << v << std::endl;
                return 1;
            }
            double r = noise1.ridged2d(x, y);
            if (r < -0.01 || r > 1.05) {
                std::cerr << "FAILED: ridged2d out of range [0, 1]: " << r << std::endl;
                return 1;
            }
        }
    }

    std::cout << "[PASS] test_world_noise passed successfully." << std::endl;
    return 0;
}
