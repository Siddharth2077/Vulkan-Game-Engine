#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


/// @brief Holds the data pertaining to an image allocated using VMA allocator
///
/// This structure encapsulates all the necessary handles and metadata for a complete
/// Vulkan image resource managed by VMA (Vulkan Memory Allocator).
///
/// @param image        The VkImage handle representing the GPU image resource
/// @param imageView    The VkImageView handle for accessing the image in shaders
/// @param vmaAllocation The VMA allocation handle for automatic memory management
/// @param imageExtent  The 3D dimensions (width, height, depth) of the image
/// @param imageFormat  The pixel format specification (ex., VK_FORMAT_R8G8B8A8_UNORM)
struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation vmaAllocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};