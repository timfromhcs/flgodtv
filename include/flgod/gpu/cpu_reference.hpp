#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace flgod {

class CpuReference {
public:
    static void vector_add(const float* a, const float* b, float* c, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            c[i] = a[i] + b[i];
        }
    }

    static void field_step_2d(const float* in, float* out, uint32_t width, uint32_t height, float diffusion_rate, float dt) {
        for (uint32_t z = 0; z < height; ++z) {
            for (uint32_t x = 0; x < width; ++x) {
                uint32_t idx = z * width + x;
                float center = in[idx];

                uint32_t left_x  = (x > 0) ? (x - 1) : 0;
                uint32_t right_x = (x + 1 < width) ? (x + 1) : (width - 1);
                uint32_t up_z    = (z > 0) ? (z - 1) : 0;
                uint32_t down_z  = (z + 1 < height) ? (z + 1) : (height - 1);

                float left  = in[z * width + left_x];
                float right = in[z * width + right_x];
                float up    = in[up_z * width + x];
                float down  = in[down_z * width + x];

                float laplacian = (left + right + up + down) - 4.0f * center;
                out[idx] = center + diffusion_rate * laplacian * dt;
            }
        }
    }

    static bool compare_buffers(const float* cpu, const float* gpu, size_t count, float tolerance = 1e-4f, size_t* out_mismatch_idx = nullptr, float* out_max_diff = nullptr) {
        float max_diff = 0.0f;
        bool match = true;
        for (size_t i = 0; i < count; ++i) {
            float diff = std::abs(cpu[i] - gpu[i]);
            if (diff > max_diff) {
                max_diff = diff;
            }
            if (diff > tolerance) {
                if (out_mismatch_idx && match) {
                    *out_mismatch_idx = i;
                }
                match = false;
            }
        }
        if (out_max_diff) {
            *out_max_diff = max_diff;
        }
        return match;
    }
};

} // namespace flgod
