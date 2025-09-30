#pragma once

#include "vk_types.h"

struct DescriptorLayoutBuilder {
private:
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

public:
    void add_binding(uint32_t binding, VkDescriptorType descriptor_type);
    void clear_all_bindings();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStageFlags, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags = 0);
};


struct DescriptorSetAllocator {
private:
    struct PoolSizeRatio {
        VkDescriptorType descriptor_type;
        float ratio;
    };
    VkDescriptorPool descriptor_pool;

public:
    void init_descriptor_pool(VkDevice device, uint32_t max_descriptor_sets, std::span<PoolSizeRatio> pool_size_ratios);
    void clear_all_descriptor_sets(VkDevice device);
    void destroy_descriptor_pool(VkDevice device);

    VkDescriptorSet allocate_descriptor_set(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);
};
