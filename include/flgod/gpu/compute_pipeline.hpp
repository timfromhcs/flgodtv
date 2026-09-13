#pragma once

#include "flgod/gpu/vulkan_context.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

namespace flgod {

class ComputePipeline {
public:
    ComputePipeline() = default;
    ~ComputePipeline() {
        destroy();
    }

    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(ComputePipeline&& other) noexcept {
        move_from(std::move(other));
    }
    ComputePipeline& operator=(ComputePipeline&& other) noexcept {
        if (this != &other) {
            destroy();
            move_from(std::move(other));
        }
        return *this;
    }

    bool create_from_file(const VulkanContext& ctx, const std::string& spv_path, uint32_t buffer_count, size_t push_constant_size = 0) {
        std::ifstream file(spv_path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ComputePipeline] Failed to open SPIR-V file: " << spv_path << std::endl;
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        return create_from_spv(ctx, buffer, buffer_count, push_constant_size);
    }

    bool create_from_spv(const VulkanContext& ctx, const std::vector<uint32_t>& spv_code, uint32_t buffer_count, size_t push_constant_size = 0) {
        destroy();
        m_device = ctx.device();
        m_buffer_count = buffer_count;
        m_push_constant_size = push_constant_size;

        // 1. Create Shader Module
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = spv_code.size() * sizeof(uint32_t);
        create_info.pCode = spv_code.data();

        if (vkCreateShaderModule(m_device, &create_info, nullptr, &m_shader_module) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to create shader module!" << std::endl;
            return false;
        }

        // 2. Create Descriptor Set Layout
        std::vector<VkDescriptorSetLayoutBinding> bindings(buffer_count);
        for (uint32_t i = 0; i < buffer_count; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descriptor_layout) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to create descriptor set layout!" << std::endl;
            destroy();
            return false;
        }

        // 3. Create Pipeline Layout
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_descriptor_layout;

        VkPushConstantRange pc_range{};
        if (push_constant_size > 0) {
            pc_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            pc_range.offset = 0;
            pc_range.size = static_cast<uint32_t>(push_constant_size);
            pipeline_layout_info.pushConstantRangeCount = 1;
            pipeline_layout_info.pPushConstantRanges = &pc_range;
        }

        if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to create pipeline layout!" << std::endl;
            destroy();
            return false;
        }

        // 4. Create Compute Pipeline
        VkComputePipelineCreateInfo comp_info{};
        comp_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        comp_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        comp_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        comp_info.stage.module = m_shader_module;
        comp_info.stage.pName = "main";
        comp_info.layout = m_pipeline_layout;

        if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &comp_info, nullptr, &m_pipeline) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to create compute pipeline!" << std::endl;
            destroy();
            return false;
        }

        // 5. Create Descriptor Pool
        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_size.descriptorCount = buffer_count;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;

        if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to create descriptor pool!" << std::endl;
            destroy();
            return false;
        }

        // 6. Allocate Descriptor Set
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_layout;

        if (vkAllocateDescriptorSets(m_device, &alloc_info, &m_descriptor_set) != VK_SUCCESS) {
            std::cerr << "[ComputePipeline] Failed to allocate descriptor set!" << std::endl;
            destroy();
            return false;
        }

        return true;
    }

    bool bind_buffers(const std::vector<VkBuffer>& buffers) {
        if (buffers.size() != m_buffer_count || m_descriptor_set == VK_NULL_HANDLE) {
            return false;
        }

        std::vector<VkDescriptorBufferInfo> buffer_infos(buffers.size());
        std::vector<VkWriteDescriptorSet> writes(buffers.size());

        for (size_t i = 0; i < buffers.size(); ++i) {
            buffer_infos[i].buffer = buffers[i];
            buffer_infos[i].offset = 0;
            buffer_infos[i].range = VK_WHOLE_SIZE;

            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = m_descriptor_set;
            writes[i].dstBinding = static_cast<uint32_t>(i);
            writes[i].dstArrayElement = 0;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[i].descriptorCount = 1;
            writes[i].pBufferInfo = &buffer_infos[i];
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        return true;
    }

    bool dispatch(const VulkanContext& ctx, uint32_t group_x, uint32_t group_y = 1, uint32_t group_z = 1,
                  const void* push_constants = nullptr, size_t push_constant_size = 0) {
        if (m_pipeline == VK_NULL_HANDLE) return false;

        // Allocate a single-use command buffer
        VkCommandBufferAllocateInfo cmd_alloc{};
        cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc.commandPool = ctx.command_pool();
        cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc.commandBufferCount = 1;

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(m_device, &cmd_alloc, &cmd) != VK_SUCCESS) {
            return false;
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &begin_info);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

        if (push_constants != nullptr && push_constant_size > 0) {
            vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(push_constant_size), push_constants);
        }

        vkCmdDispatch(cmd, group_x, group_y, group_z);

        vkEndCommandBuffer(cmd);

        // Submit and wait for fence
        VkFence fence = VK_NULL_HANDLE;
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(m_device, &fence_info, nullptr, &fence);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;

        vkQueueSubmit(ctx.compute_queue(), 1, &submit_info, fence);
        vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, ctx.command_pool(), 1, &cmd);

        return true;
    }

    void destroy() noexcept {
        if (m_device != VK_NULL_HANDLE) {
            if (m_descriptor_pool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
                m_descriptor_pool = VK_NULL_HANDLE;
            }
            if (m_pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_device, m_pipeline, nullptr);
                m_pipeline = VK_NULL_HANDLE;
            }
            if (m_pipeline_layout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
                m_pipeline_layout = VK_NULL_HANDLE;
            }
            if (m_descriptor_layout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(m_device, m_descriptor_layout, nullptr);
                m_descriptor_layout = VK_NULL_HANDLE;
            }
            if (m_shader_module != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_device, m_shader_module, nullptr);
                m_shader_module = VK_NULL_HANDLE;
            }
            m_device = VK_NULL_HANDLE;
        }
        m_descriptor_set = VK_NULL_HANDLE;
        m_buffer_count = 0;
        m_push_constant_size = 0;
    }

private:
    void move_from(ComputePipeline&& other) noexcept {
        m_device = other.m_device;
        m_shader_module = other.m_shader_module;
        m_descriptor_layout = other.m_descriptor_layout;
        m_pipeline_layout = other.m_pipeline_layout;
        m_pipeline = other.m_pipeline;
        m_descriptor_pool = other.m_descriptor_pool;
        m_descriptor_set = other.m_descriptor_set;
        m_buffer_count = other.m_buffer_count;
        m_push_constant_size = other.m_push_constant_size;

        other.m_device = VK_NULL_HANDLE;
        other.m_shader_module = VK_NULL_HANDLE;
        other.m_descriptor_layout = VK_NULL_HANDLE;
        other.m_pipeline_layout = VK_NULL_HANDLE;
        other.m_pipeline = VK_NULL_HANDLE;
        other.m_descriptor_pool = VK_NULL_HANDLE;
        other.m_descriptor_set = VK_NULL_HANDLE;
        other.m_buffer_count = 0;
        other.m_push_constant_size = 0;
    }

    VkDevice m_device{VK_NULL_HANDLE};
    VkShaderModule m_shader_module{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_descriptor_layout{VK_NULL_HANDLE};
    VkPipelineLayout m_pipeline_layout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};
    VkDescriptorSet m_descriptor_set{VK_NULL_HANDLE};
    uint32_t m_buffer_count{0};
    size_t m_push_constant_size{0};
};

} // namespace flgod
