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
    /// @brief Describe the @code ratio@endcode of each type of Descriptor to allocate from the Pool.
    /// @note Suppose @code max_descriptor_sets@endcode = 100, and @code ratio@endcode = 0.5 for a given @code descriptor_type@endcode, 50 such descriptors will be allocated in the Pool
    /// @attention This ratio is against the @code max_descriptor_sets@endcode value passed during @code init_descriptor_pool()@endcode
    struct PoolSizeRatio {
        VkDescriptorType descriptor_type;
        float ratio;
    };

private:
    VkDescriptorPool descriptor_pool {VK_NULL_HANDLE};

public:
    void init_descriptor_pool(VkDevice device, uint32_t max_descriptor_sets, std::span<PoolSizeRatio> pool_size_ratios);
    void clear_all_descriptor_sets(VkDevice device);
    void destroy_descriptor_pool(VkDevice device);

    VkDescriptorSet allocate_descriptor_set(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);
};
