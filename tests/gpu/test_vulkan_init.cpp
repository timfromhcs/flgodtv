#include "flgod/gpu/vulkan_context.hpp"
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_vulkan_init..." << std::endl;

    flgod::VulkanContext ctx;
    bool ok = ctx.initialize();
    if (!ok) {
        std::cerr << "FAILED: VulkanContext initialization failed!" << std::endl;
        return 1;
    }

    const auto& info = ctx.device_info();
    std::cout << "  Device Name: " << info.device_name << "\n"
              << "  Vendor ID: 0x" << std::hex << info.vendor_id 
              << ", Device ID: 0x" << info.device_id << std::dec << "\n"
              << "  API Version: " << VK_VERSION_MAJOR(info.api_version) << "."
              << VK_VERSION_MINOR(info.api_version) << "."
              << VK_VERSION_PATCH(info.api_version) << "\n"
              << "  Compute Queue Family: " << ctx.compute_queue_family_index() << "\n"
              << "  Total Memory: " << (info.total_memory_bytes / (1024 * 1024)) << " MB\n";

    if (info.device_name.empty()) {
        std::cerr << "FAILED: Device name is empty!" << std::endl;
        return 1;
    }

    ctx.cleanup();
    if (ctx.is_initialized()) {
        std::cerr << "FAILED: Context reported initialized after cleanup!" << std::endl;
        return 1;
    }

    std::cout << "[TEST] test_vulkan_init PASSED" << std::endl;
    return 0;
}
