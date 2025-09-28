#include "vk_images.h"

void vkutil::transition_image_layout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) {

    // Specify the image memory barrier (and the subresource range)
    VkImageMemoryBarrier2 imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcStageMask = srcStageMask;
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstStageMask = dstStageMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;

    VkImageSubresourceRange imageSubresourceRange {};
    imageSubresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
    imageSubresourceRange.baseMipLevel = 0;
    imageSubresourceRange.levelCount = 1;
    imageSubresourceRange.baseArrayLayer = 0;
    imageSubresourceRange.layerCount = 1;

    imageMemoryBarrier.subresourceRange = imageSubresourceRange;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.oldLayout = currentLayout;
    imageMemoryBarrier.newLayout = newLayout;

    // Specify the dependency info struct:
    VkDependencyInfo dependencyInfo {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier;

    // Call the command to place a Pipeline Barrier:
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void vkutil::blit_image_to_image(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImage dstImage, VkExtent2D srcImageExtent, VkExtent2D dstImageExtent) {
    // Specify the image blit operation:
    VkImageBlit2 image_blit_region{};
    image_blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    image_blit_region.pNext = nullptr;
    image_blit_region.srcOffsets[0] = { 0, 0, 0 };                  // Src [0] - Top-Left Corner
    image_blit_region.srcOffsets[1].x = srcImageExtent.width;       // Src [1] - Bottom-Right Corner
    image_blit_region.srcOffsets[1].y = srcImageExtent.height;
    image_blit_region.srcOffsets[1].z = 1;
    image_blit_region.dstOffsets[0] = { 0, 0, 0 };                  // Dst [0] - Top-Left Corner
    image_blit_region.dstOffsets[1].x = dstImageExtent.width;       // Dst [1] - Bottom-Right Corner
    image_blit_region.dstOffsets[1].y = dstImageExtent.height;
    image_blit_region.dstOffsets[1].z = 1;
    image_blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.srcSubresource.baseArrayLayer = 0;
    image_blit_region.srcSubresource.layerCount = 1;
    image_blit_region.srcSubresource.mipLevel = 0;
    image_blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.dstSubresource.baseArrayLayer = 0;
    image_blit_region.dstSubresource.layerCount = 1;
    image_blit_region.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitImageInfo{};
    blitImageInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitImageInfo.pNext = nullptr;
    blitImageInfo.srcImage = srcImage;
    blitImageInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitImageInfo.dstImage = dstImage;
    blitImageInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitImageInfo.filter = VK_FILTER_LINEAR;
    blitImageInfo.regionCount = 1;
    blitImageInfo.pRegions = &image_blit_region;

    // Blit the image
    vkCmdBlitImage2(cmdBuffer, &blitImageInfo);
}
