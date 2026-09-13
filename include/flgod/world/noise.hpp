#pragma once

#include <cstdint>
#include <cmath>
#include <array>

namespace flgod {

class DeterministicNoise {
public:
    explicit DeterministicNoise(uint64_t seed = 133701ULL) {
        init(seed);
    }

    void init(uint64_t seed) noexcept {
        uint64_t sm = seed;
        auto splitmix = [&sm]() -> uint64_t {
            uint64_t z = (sm += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        };

        // Initialize permutation table
        for (int i = 0; i < 256; ++i) {
            m_p[i] = static_cast<uint8_t>(i);
        }
        // Fisher-Yates shuffle
        for (int i = 255; i > 0; --i) {
            int j = static_cast<int>(splitmix() % static_cast<uint64_t>(i + 1));
            std::swap(m_p[i], m_p[j]);
        }
        // Duplicate for overflow-free indexing
        for (int i = 0; i < 256; ++i) {
            m_perm[i] = m_p[i];
            m_perm[256 + i] = m_p[i];
        }
    }

    // 2D Perlin noise in [-1.0, 1.0]
    [[nodiscard]] double noise2d(double x, double y) const noexcept {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        double u = fade(x);
        double v = fade(y);

        int A = m_perm[X] + Y;
        int B = m_perm[X + 1] + Y;

        double g00 = grad2d(m_perm[A], x, y);
        double g10 = grad2d(m_perm[B], x - 1.0, y);
        double g01 = grad2d(m_perm[A + 1], x, y - 1.0);
        double g11 = grad2d(m_perm[B + 1], x - 1.0, y - 1.0);

        double top = lerp(u, g00, g10);
        double bottom = lerp(u, g01, g11);
        return lerp(v, top, bottom) * 0.7071067811865475;
    }

    // 3D Perlin noise in [-1.0, 1.0]
    [[nodiscard]] double noise3d(double x, double y, double z) const noexcept {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A = m_perm[X] + Y;
        int AA = m_perm[A] + Z;
        int AB = m_perm[A + 1] + Z;
        int B = m_perm[X + 1] + Y;
        int BA = m_perm[B] + Z;
        int BB = m_perm[B + 1] + Z;

        double g000 = grad3d(m_perm[AA], x, y, z);
        double g100 = grad3d(m_perm[BA], x - 1.0, y, z);
        double g010 = grad3d(m_perm[AB], x, y - 1.0, z);
        double g110 = grad3d(m_perm[BB], x - 1.0, y - 1.0, z);
        double g001 = grad3d(m_perm[AA + 1], x, y, z - 1.0);
        double g101 = grad3d(m_perm[BA + 1], x - 1.0, y, z - 1.0);
        double g011 = grad3d(m_perm[AB + 1], x, y - 1.0, z - 1.0);
        double g111 = grad3d(m_perm[BB + 1], x - 1.0, y - 1.0, z - 1.0);

        return lerp(w,
            lerp(v, lerp(u, g000, g100), lerp(u, g010, g110)),
            lerp(v, lerp(u, g001, g101), lerp(u, g011, g111))
        );
    }

    // Fractal Brownian Motion (fBm) in [-1.0, 1.0] approx
    [[nodiscard]] double fbm2d(double x, double y, int octaves = 6, double lacunarity = 2.0, double gain = 0.5) const noexcept {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double max_value = 0.0;

        for (int i = 0; i < octaves; ++i) {
            total += noise2d(x * frequency, y * frequency) * amplitude;
            max_value += amplitude;
            frequency *= lacunarity;
            amplitude *= gain;
        }
        return total / max_value;
    }

    // Ridged multifractal noise (sharp ridges for mountains) in [0.0, 1.0]
    [[nodiscard]] double ridged2d(double x, double y, int octaves = 4, double lacunarity = 2.0, double gain = 0.5) const noexcept {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double max_value = 0.0;

        for (int i = 0; i < octaves; ++i) {
            double n = std::abs(noise2d(x * frequency, y * frequency));
            n = 1.0 - n; // invert
            n = n * n;   // sharpen
            total += n * amplitude;
            max_value += amplitude;
            frequency *= lacunarity;
            amplitude *= gain;
        }
        return total / max_value;
    }

private:
    static constexpr double fade(double t) noexcept {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static constexpr double lerp(double t, double a, double b) noexcept {
        return a + t * (b - a);
    }

    static constexpr double grad2d(int hash, double x, double y) noexcept {
        int h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
    }

    static constexpr double grad3d(int hash, double x, double y, double z) noexcept {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    std::array<uint8_t, 256> m_p{};
    std::array<uint8_t, 512> m_perm{};
};

} // namespace flgod
