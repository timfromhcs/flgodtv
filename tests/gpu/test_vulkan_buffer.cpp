#include "flgod/gpu/compute_buffer.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "[TEST] Running test_vulkan_buffer..." << std::endl;

    flgod::VulkanContext ctx;
    if (!ctx.initialize()) {
        std::cerr << "FAILED: Failed to initialize VulkanContext" << std::endl;
        return 1;
    }

    const size_t test_size = 1024 * 1024; // 1 MB
    flgod::ComputeBuffer buffer;
    bool created = buffer.create(ctx, test_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!created) {
        std::cerr << "FAILED: Failed to create 1 MB ComputeBuffer" << std::endl;
        return 1;
    }

    // 1. Upload pattern and read back
    std::vector<uint8_t> src(test_size);
    for (size_t i = 0; i < test_size; ++i) {
        src[i] = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
    }

    if (!buffer.upload(src.data(), test_size)) {
        std::cerr << "FAILED: Buffer upload failed!" << std::endl;
        return 1;
    }

    std::vector<uint8_t> dst(test_size, 0);
    if (!buffer.download(dst.data(), test_size)) {
        std::cerr << "FAILED: Buffer download failed!" << std::endl;
        return 1;
    }

    for (size_t i = 0; i < test_size; ++i) {
        if (src[i] != dst[i]) {
            std::cerr << "FAILED: Buffer mismatch at byte " << i << ": expected " 
                      << static_cast<int>(src[i]) << ", got " << static_cast<int>(dst[i]) << std::endl;
            return 1;
        }
    }
    std::cout << "  1 MB upload/download pattern match verified." << std::endl;

    // 2. Memory transfer benchmark (GEMINI.md Section 39)
    auto bench = flgod::ComputeBuffer::benchmark_transfer(ctx, 4 * 1024 * 1024, 30); // 4 MB, 30 runs
    std::cout << "  Memory Transfer Benchmark (4 MB payload):\n"
              << "    Host -> Device: " << bench.host_to_device_mb_per_sec << " MB/s (" << bench.upload_ms << " ms)\n"
              << "    Device -> Host: " << bench.device_to_host_mb_per_sec << " MB/s (" << bench.download_ms << " ms)\n";

    if (bench.host_to_device_mb_per_sec <= 0.0 || bench.device_to_host_mb_per_sec <= 0.0) {
        std::cerr << "FAILED: Invalid transfer benchmark numbers reported!" << std::endl;
        return 1;
    }

    std::cout << "[TEST] test_vulkan_buffer PASSED" << std::endl;
    return 0;
}
