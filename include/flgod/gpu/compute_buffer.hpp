#pragma once

#include "flgod/gpu/vulkan_context.hpp"
#include <chrono>
#include <cstring>

namespace flgod {

struct TransferBenchmarkResult {
    double host_to_device_mb_per_sec{0.0};
    double device_to_host_mb_per_sec{0.0};
    size_t transfer_size_bytes{0};
    double upload_ms{0.0};
    double download_ms{0.0};
};

class ComputeBuffer {
public:
    ComputeBuffer() = default;
    ~ComputeBuffer() {
        destroy();
    }

    ComputeBuffer(const ComputeBuffer&) = delete;
    ComputeBuffer& operator=(const ComputeBuffer&) = delete;

    ComputeBuffer(ComputeBuffer&& other) noexcept {
        move_from(std::move(other));
    }
    ComputeBuffer& operator=(ComputeBuffer&& other) noexcept {
        if (this != &other) {
            destroy();
            move_from(std::move(other));
        }
        return *this;
    }

    bool create(const VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        destroy();
        m_device = ctx.device();
        m_size = size;
        m_properties = properties;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &buffer_info, nullptr, &m_buffer) != VK_SUCCESS) {
            std::cerr << "[ComputeBuffer] Failed to create VkBuffer!" << std::endl;
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(m_device, m_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = ctx.find_memory_type(mem_reqs.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device, &alloc_info, nullptr, &m_memory) != VK_SUCCESS) {
            std::cerr << "[ComputeBuffer] Failed to allocate device memory for buffer!" << std::endl;
            destroy();
            return false;
        }

        if (vkBindBufferMemory(m_device, m_buffer, m_memory, 0) != VK_SUCCESS) {
            std::cerr << "[ComputeBuffer] Failed to bind buffer memory!" << std::endl;
            destroy();
            return false;
        }

        return true;
    }

    void destroy() noexcept {
        if (m_device != VK_NULL_HANDLE) {
            if (m_mapped_ptr != nullptr) {
                vkUnmapMemory(m_device, m_memory);
                m_mapped_ptr = nullptr;
            }
            if (m_buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_device, m_buffer, nullptr);
                m_buffer = VK_NULL_HANDLE;
            }
            if (m_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device, m_memory, nullptr);
                m_memory = VK_NULL_HANDLE;
            }
            m_device = VK_NULL_HANDLE;
        }
        m_size = 0;
    }

    bool upload(const void* data, size_t size, size_t offset = 0) {
        if (m_device == VK_NULL_HANDLE || m_memory == VK_NULL_HANDLE) return false;
        if (offset + size > m_size) return false;

        void* mapped = nullptr;
        if (vkMapMemory(m_device, m_memory, offset, size, 0, &mapped) != VK_SUCCESS) {
            return false;
        }
        std::memcpy(mapped, data, size);

        // If not host coherent, flush mapped memory range
        if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range{};
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.memory = m_memory;
            range.offset = offset;
            range.size = size;
            vkFlushMappedMemoryRanges(m_device, 1, &range);
        }

        vkUnmapMemory(m_device, m_memory);
        return true;
    }

    bool download(void* data, size_t size, size_t offset = 0) const {
        if (m_device == VK_NULL_HANDLE || m_memory == VK_NULL_HANDLE) return false;
        if (offset + size > m_size) return false;

        void* mapped = nullptr;
        if (vkMapMemory(m_device, m_memory, offset, size, 0, &mapped) != VK_SUCCESS) {
            return false;
        }

        // If not host coherent, invalidate mapped memory range
        if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range{};
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.memory = m_memory;
            range.offset = offset;
            range.size = size;
            vkInvalidateMappedMemoryRanges(m_device, 1, &range);
        }

        std::memcpy(data, mapped, size);
        vkUnmapMemory(m_device, m_memory);
        return true;
    }

    [[nodiscard]] VkBuffer buffer() const noexcept { return m_buffer; }
    [[nodiscard]] VkDeviceMemory memory() const noexcept { return m_memory; }
    [[nodiscard]] VkDeviceSize size() const noexcept { return m_size; }
    [[nodiscard]] bool is_valid() const noexcept { return m_buffer != VK_NULL_HANDLE; }

    static TransferBenchmarkResult benchmark_transfer(const VulkanContext& ctx, size_t bytes, size_t iterations = 20) {
        TransferBenchmarkResult res{};
        res.transfer_size_bytes = bytes;

        ComputeBuffer buf;
        bool ok = buf.create(ctx, bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!ok) return res;

        std::vector<uint8_t> host_data(bytes, 0xAA);
        std::vector<uint8_t> readback_data(bytes, 0x00);

        // Benchmark Host to Device
        auto start_up = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            buf.upload(host_data.data(), bytes);
        }
        auto end_up = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> up_duration = end_up - start_up;
        res.upload_ms = up_duration.count() / iterations;
        double total_mb_up = (static_cast<double>(bytes * iterations)) / (1024.0 * 1024.0);
        res.host_to_device_mb_per_sec = total_mb_up / (up_duration.count() / 1000.0);

        // Benchmark Device to Host
        auto start_down = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            buf.download(readback_data.data(), bytes);
        }
        auto end_down = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> down_duration = end_down - start_down;
        res.download_ms = down_duration.count() / iterations;
        double total_mb_down = (static_cast<double>(bytes * iterations)) / (1024.0 * 1024.0);
        res.device_to_host_mb_per_sec = total_mb_down / (down_duration.count() / 1000.0);

        return res;
    }

private:
    void move_from(ComputeBuffer&& other) noexcept {
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_properties = other.m_properties;
        m_mapped_ptr = other.m_mapped_ptr;

        other.m_device = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mapped_ptr = nullptr;
    }

    VkDevice m_device{VK_NULL_HANDLE};
    VkBuffer m_buffer{VK_NULL_HANDLE};
    VkDeviceMemory m_memory{VK_NULL_HANDLE};
    VkDeviceSize m_size{0};
    VkMemoryPropertyFlags m_properties{0};
    void* m_mapped_ptr{nullptr};
};

} // namespace flgod
