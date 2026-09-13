#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <memory>

namespace flgod {

struct DeviceInfo {
    std::string device_name;
    uint32_t device_id{0};
    uint32_t vendor_id{0};
    uint32_t api_version{0};
    uint32_t driver_version{0};
    VkPhysicalDeviceType device_type{VK_PHYSICAL_DEVICE_TYPE_OTHER};
    uint64_t total_memory_bytes{0};
};

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext() {
        cleanup();
    }

    // Non-copyable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // Moveable
    VulkanContext(VulkanContext&& other) noexcept {
        move_from(std::move(other));
    }
    VulkanContext& operator=(VulkanContext&& other) noexcept {
        if (this != &other) {
            cleanup();
            move_from(std::move(other));
        }
        return *this;
    }

    bool initialize(bool enable_validation = false) {
        if (m_initialized) return true;

        // 1. Create Vulkan Instance
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "FLGODTV Vulkan Compute Engine";
        app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.pEngineName = "FLGOD";
        app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo inst_info{};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pApplicationInfo = &app_info;

        std::vector<const char*> layers;
        if (enable_validation) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
        inst_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        inst_info.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

        VkResult res = vkCreateInstance(&inst_info, nullptr, &m_instance);
        if (res != VK_SUCCESS) {
            // Fallback without validation layer if validation was requested but layer is missing
            if (enable_validation) {
                inst_info.enabledLayerCount = 0;
                inst_info.ppEnabledLayerNames = nullptr;
                res = vkCreateInstance(&inst_info, nullptr, &m_instance);
            }
            if (res != VK_SUCCESS) {
                std::cerr << "[VulkanContext] Failed to create Vulkan instance, error: " << res << std::endl;
                return false;
            }
        }

        // 2. Select Physical Device with Compute Queue
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        if (device_count == 0) {
            std::cerr << "[VulkanContext] No Vulkan-compatible physical devices found!" << std::endl;
            cleanup();
            return false;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

        m_physical_device = VK_NULL_HANDLE;
        for (const auto& dev : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);

            // Look for queue family with compute support
            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families.data());

            for (uint32_t i = 0; i < queue_family_count; ++i) {
                if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    m_physical_device = dev;
                    m_compute_queue_family_index = i;
                    m_device_info.device_name = props.deviceName;
                    m_device_info.device_id = props.deviceID;
                    m_device_info.vendor_id = props.vendorID;
                    m_device_info.api_version = props.apiVersion;
                    m_device_info.driver_version = props.driverVersion;
                    m_device_info.device_type = props.deviceType;
                    break;
                }
            }

            if (m_physical_device != VK_NULL_HANDLE) {
                // If it's discrete GPU, prioritize it; otherwise integrated GPU is also fine
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    break;
                }
            }
        }

        if (m_physical_device == VK_NULL_HANDLE) {
            std::cerr << "[VulkanContext] No device with compute queue capability found!" << std::endl;
            cleanup();
            return false;
        }

        // Query memory properties
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_memory_properties);
        for (uint32_t i = 0; i < m_memory_properties.memoryHeapCount; ++i) {
            m_device_info.total_memory_bytes += m_memory_properties.memoryHeaps[i].size;
        }

        // 3. Create Logical Device and Compute Queue
        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = m_compute_queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        VkDeviceCreateInfo dev_info{};
        dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dev_info.pQueueCreateInfos = &queue_create_info;
        dev_info.queueCreateInfoCount = 1;

        res = vkCreateDevice(m_physical_device, &dev_info, nullptr, &m_device);
        if (res != VK_SUCCESS) {
            std::cerr << "[VulkanContext] Failed to create logical device: " << res << std::endl;
            cleanup();
            return false;
        }

        vkGetDeviceQueue(m_device, m_compute_queue_family_index, 0, &m_compute_queue);

        // 4. Create Command Pool
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = m_compute_queue_family_index;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        res = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool);
        if (res != VK_SUCCESS) {
            std::cerr << "[VulkanContext] Failed to create command pool: " << res << std::endl;
            cleanup();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void cleanup() noexcept {
        if (m_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_device);
            if (m_command_pool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_device, m_command_pool, nullptr);
                m_command_pool = VK_NULL_HANDLE;
            }
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
        m_physical_device = VK_NULL_HANDLE;
        m_compute_queue = VK_NULL_HANDLE;
        m_compute_queue_family_index = 0;
        m_initialized = false;
    }

    [[nodiscard]] bool is_initialized() const noexcept { return m_initialized; }
    [[nodiscard]] VkInstance instance() const noexcept { return m_instance; }
    [[nodiscard]] VkPhysicalDevice physical_device() const noexcept { return m_physical_device; }
    [[nodiscard]] VkDevice device() const noexcept { return m_device; }
    [[nodiscard]] VkQueue compute_queue() const noexcept { return m_compute_queue; }
    [[nodiscard]] uint32_t compute_queue_family_index() const noexcept { return m_compute_queue_family_index; }
    [[nodiscard]] VkCommandPool command_pool() const noexcept { return m_command_pool; }
    [[nodiscard]] const DeviceInfo& device_info() const noexcept { return m_device_info; }
    [[nodiscard]] const VkPhysicalDeviceMemoryProperties& memory_properties() const noexcept { return m_memory_properties; }

    [[nodiscard]] uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const {
        for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; ++i) {
            if ((type_filter & (1 << i)) &&
                (m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable Vulkan memory type!");
    }

private:
    void move_from(VulkanContext&& other) noexcept {
        m_instance = other.m_instance;
        m_physical_device = other.m_physical_device;
        m_device = other.m_device;
        m_compute_queue = other.m_compute_queue;
        m_compute_queue_family_index = other.m_compute_queue_family_index;
        m_command_pool = other.m_command_pool;
        m_memory_properties = other.m_memory_properties;
        m_device_info = std::move(other.m_device_info);
        m_initialized = other.m_initialized;

        other.m_instance = VK_NULL_HANDLE;
        other.m_physical_device = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
        other.m_compute_queue = VK_NULL_HANDLE;
        other.m_command_pool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }

    VkInstance m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physical_device{VK_NULL_HANDLE};
    VkDevice m_device{VK_NULL_HANDLE};
    VkQueue m_compute_queue{VK_NULL_HANDLE};
    uint32_t m_compute_queue_family_index{0};
    VkCommandPool m_command_pool{VK_NULL_HANDLE};
    VkPhysicalDeviceMemoryProperties m_memory_properties{};
    DeviceInfo m_device_info{};
    bool m_initialized{false};
};

} // namespace flgod
