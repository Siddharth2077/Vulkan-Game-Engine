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

}

void DescriptorSetAllocator::clear_all_descriptor_sets(VkDevice device) {

}

void DescriptorSetAllocator::destroy_descriptor_pool(VkDevice device) {

}

VkDescriptorSet DescriptorSetAllocator::allocate_descriptor_set(VkDevice device, VkDescriptorSetLayout descriptor_set_layout) {

}


