#pragma once

#include "vk_types.h"

namespace vkutil {

    /// @brief Transitions an image from its current image-layout to a new layout.
    /// @attention Stage and Access masks are given a safe (unoptimized) default value.
    /// @note Places a pipeline-barrier by calling @code vkCmdPipelineBarrier2@endcode
    void transition_image_layout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout currentLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
    );

    /// @brief Uses the command @code vkCmdBlitImage2@endcode to blit-copy the source image, onto the destination image.
    void blit_image_to_image(
        VkCommandBuffer cmdBuffer,
        VkImage srcImage,
        VkImage dstImage,
        VkExtent2D srcImageExtent,
        VkExtent2D dstImageExtent
    );

};