#include "vk_descriptors.h"
#include "vk_logger.h"

// Descriptor Layout Builder: Method Definitions

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType descriptor_type) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;
    newBinding.descriptorType = descriptor_type;
    newBinding.descriptorCount = 1;
    // The stageFlags are set when building the VkDescriptorSetLayout [in the build() function]

    layout_bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear_all_bindings() {
    layout_bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStageFlags, void *pNext, VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags) {
    // Set the stage flags for each binding
    for (auto& binding : layout_bindings) {
        binding.stageFlags |= shaderStageFlags;
    }

    // Create the Descriptor Set Layout
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pNext = pNext;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layoutCreateInfo.pBindings = layout_bindings.data();
    layoutCreateInfo.flags = descriptorSetLayoutCreateFlags;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptor_set_layout);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create descriptor set-layout!");
        throw std::runtime_error("Failed to create descriptor set-layout!");
    }
    VK_LOG_SUCCESS("Created descriptor set-layout");

    return descriptor_set_layout;
}


// Descriptor Set Allocator: Method Definitions

void DescriptorSetAllocator::init_descriptor_pool(VkDevice device, uint32_t max_descriptor_sets, std::span<PoolSizeRatio> pool_size_ratios) {
    // Create the descriptor pool sizes, based on the ratios and max_descriptor_sets value provided:
    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes{};
    for (PoolSizeRatio pool_size : pool_size_ratios) {
        descriptor_pool_sizes.push_back(
            VkDescriptorPoolSize{
                .type = pool_size.descriptor_type,
                .descriptorCount = static_cast<uint32_t>(max_descriptor_sets * pool_size.ratio)
            }
        );
    }

    // Create the descriptor-pool:
    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = 0;
    pool_create_info.maxSets = max_descriptor_sets;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    pool_create_info.pPoolSizes = descriptor_pool_sizes.data();

    // Create the descriptor-pool and store it in the struct
    VkResult result = vkCreateDescriptorPool(device, &pool_create_info, nullptr, &descriptor_pool);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create descriptor pool!");
        throw std::runtime_error("Failed to create descriptor pool!");
    }
    VK_LOG_SUCCESS("Created descriptor pool");
}

void DescriptorSetAllocator::clear_all_descriptor_sets(VkDevice device) {
    // Resetting the descriptor-pool destroys all the descriptor-sets creates from it.
    vkResetDescriptorPool(device, descriptor_pool, 0);
}

void DescriptorSetAllocator::destroy_descriptor_pool(VkDevice device) {
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
}

VkDescriptorSet DescriptorSetAllocator::allocate_descriptor_set(VkDevice device, VkDescriptorSetLayout descriptor_set_layout) {
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &descriptor_set_layout;

    VkDescriptorSet descriptor_set {VK_NULL_HANDLE};
    VkResult result = vkAllocateDescriptorSets(device, &allocate_info, nullptr);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to allocate descriptor set!");
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
    VK_LOG_SUCCESS("Created descriptor set");

    return descriptor_set;
}


