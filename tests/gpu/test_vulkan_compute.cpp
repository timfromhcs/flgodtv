#include "flgod/gpu/compute_pipeline.hpp"
#include "flgod/gpu/compute_buffer.hpp"
#include "flgod/gpu/cpu_reference.hpp"
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    std::cout << "[TEST] Running test_vulkan_compute..." << std::endl;

    flgod::VulkanContext ctx;
    if (!ctx.initialize()) {
        std::cerr << "FAILED: Failed to initialize VulkanContext" << std::endl;
        return 1;
    }

    // 1. Vector Addition Kernel (GEMINI.md Sections 35 & 36)
    const uint32_t count = 65536;
    const size_t byte_size = count * sizeof(float);

    flgod::ComputeBuffer buf_a, buf_b, buf_c;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (!buf_a.create(ctx, byte_size, usage, mem_props) ||
        !buf_b.create(ctx, byte_size, usage, mem_props) ||
        !buf_c.create(ctx, byte_size, usage, mem_props)) {
        std::cerr << "FAILED: Failed to create compute buffers for vector addition" << std::endl;
        return 1;
    }

    std::vector<float> host_a(count);
    std::vector<float> host_b(count);
    std::vector<float> cpu_c(count);
    std::vector<float> gpu_c(count, 0.0f);

    for (uint32_t i = 0; i < count; ++i) {
        host_a[i] = static_cast<float>(i) * 0.5f;
        host_b[i] = static_cast<float>(count - i) * 0.25f + 1.5f;
    }

    buf_a.upload(host_a.data(), byte_size);
    buf_b.upload(host_b.data(), byte_size);

    flgod::ComputePipeline pipe_vec_add;
    // Push constant structure: uint32_t count
    struct PushConstAdd {
        uint32_t count;
    } pc_add{count};

    if (!pipe_vec_add.create_from_file(ctx, "shaders/vector_add.spv", 3, sizeof(PushConstAdd))) {
        std::cerr << "FAILED: Failed to create vector_add compute pipeline from SPV" << std::endl;
        return 1;
    }

    pipe_vec_add.bind_buffers({buf_a.buffer(), buf_b.buffer(), buf_c.buffer()});

    uint32_t workgroups_x = (count + 63) / 64;
    if (!pipe_vec_add.dispatch(ctx, workgroups_x, 1, 1, &pc_add, sizeof(PushConstAdd))) {
        std::cerr << "FAILED: Failed to dispatch vector_add kernel" << std::endl;
        return 1;
    }

    buf_c.download(gpu_c.data(), byte_size);

    // Compute CPU reference
    flgod::CpuReference::vector_add(host_a.data(), host_b.data(), cpu_c.data(), count);

    size_t mismatch_idx = 0;
    float max_diff = 0.0f;
    if (!flgod::CpuReference::compare_buffers(cpu_c.data(), gpu_c.data(), count, 1e-4f, &mismatch_idx, &max_diff)) {
        std::cerr << "FAILED: Vector addition divergence between CPU and GPU! Mismatch at index "
                  << mismatch_idx << ": CPU=" << cpu_c[mismatch_idx] 
                  << ", GPU=" << gpu_c[mismatch_idx] << ", max_diff=" << max_diff << std::endl;
        return 1;
    }
    std::cout << "  Vector add (65,536 floats) verified against CPU reference. Max diff: " << max_diff << std::endl;

    // 2. World Field 2D Diffusion Kernel
    const uint32_t width = 64;
    const uint32_t height = 64;
    const uint32_t total_cells = width * height;
    const size_t field_bytes = total_cells * sizeof(float);

    flgod::ComputeBuffer buf_field_in, buf_field_out;
    if (!buf_field_in.create(ctx, field_bytes, usage, mem_props) ||
        !buf_field_out.create(ctx, field_bytes, usage, mem_props)) {
        std::cerr << "FAILED: Failed to create field compute buffers" << std::endl;
        return 1;
    }

    std::vector<float> field_in(total_cells, 0.0f);
    // Initialize with heat spot in center
    for (uint32_t z = 24; z <= 40; ++z) {
        for (uint32_t x = 24; x <= 40; ++x) {
            field_in[z * width + x] = 100.0f;
        }
    }
    buf_field_in.upload(field_in.data(), field_bytes);

    struct PushConstField {
        uint32_t width;
        uint32_t height;
        float diffusion_rate;
        float dt;
    } pc_field{width, height, 0.25f, 0.05f};

    flgod::ComputePipeline pipe_field;
    if (!pipe_field.create_from_file(ctx, "shaders/field_step.spv", 2, sizeof(PushConstField))) {
        std::cerr << "FAILED: Failed to create field_step compute pipeline from SPV" << std::endl;
        return 1;
    }

    pipe_field.bind_buffers({buf_field_in.buffer(), buf_field_out.buffer()});

    if (!pipe_field.dispatch(ctx, width / 16, height / 16, 1, &pc_field, sizeof(PushConstField))) {
        std::cerr << "FAILED: Failed to dispatch field_step kernel" << std::endl;
        return 1;
    }

    std::vector<float> gpu_field_out(total_cells, 0.0f);
    buf_field_out.download(gpu_field_out.data(), field_bytes);

    std::vector<float> cpu_field_out(total_cells, 0.0f);
    flgod::CpuReference::field_step_2d(field_in.data(), cpu_field_out.data(), width, height, pc_field.diffusion_rate, pc_field.dt);

    float field_max_diff = 0.0f;
    if (!flgod::CpuReference::compare_buffers(cpu_field_out.data(), gpu_field_out.data(), total_cells, 1e-4f, &mismatch_idx, &field_max_diff)) {
        std::cerr << "FAILED: 2D Field diffusion divergence between CPU and GPU! Mismatch at cell "
                  << mismatch_idx << ": CPU=" << cpu_field_out[mismatch_idx]
                  << ", GPU=" << gpu_field_out[mismatch_idx] << ", max_diff=" << field_max_diff << std::endl;
        return 1;
    }
    std::cout << "  2D field step (64x64 grid) verified against CPU reference. Max diff: " << field_max_diff << std::endl;

    std::cout << "[TEST] test_vulkan_compute PASSED" << std::endl;
    return 0;
}
