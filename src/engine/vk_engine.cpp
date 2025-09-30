#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "vk_engine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "vk_initializers.h"
#include "vk_types.h"
#include <chrono>
#include <thread>
#include <VkBootstrap.h>

#include "vk_images.h"
#include "vk_logger.h"
#include "vk_pipelines.h"

constexpr bool bUseValidationLayers {true};
constexpr uint64_t ENGINE_TIMEOUT_1_SECOND {1000000000};

// Global pointer to the Singleton Instance of the engine.
VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }


void VulkanEngine::init() {
    VK_LOG_INFO("Initializing VulkanEngine");
    // Only one engine initialization is allowed with the application [Singleton Instance]
    if (loadedEngine == nullptr) {
        loadedEngine = this;
    }

    // We initialize SDL and create a window with it.
    int result = SDL_Init(SDL_INIT_VIDEO);
    if (result == false) {
        VK_LOG_ERROR("Failed to initialize SDL - {}", SDL_GetError());
        throw std::runtime_error("SDL_Init failed: " + std::string(SDL_GetError()));
    }

    // SDL3: Use SDL_WINDOW_VULKAN flag directly (no cast needed)
    _window = SDL_CreateWindow(
        "Vulkan Engine",
        _windowExtent.width,
        _windowExtent.height,
        SDL_WINDOW_VULKAN  // SDL3: No cast needed, no position parameters
    );
    if (!_window) {
        VK_LOG_ERROR("Failed to create SDL window");
        throw std::runtime_error("Failed to create SDL window");
    }

    // Initialize Vulkan
    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();

    // Everything went fine
    _isInitialized = true;
    VK_LOG_SUCCESS("Initialized VulkanEngine");
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
        // Ensure that the GPU is done with all work
        vkDeviceWaitIdle(_device);

        for (int i{0}; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(_device, _frames.at(i).commandPool, nullptr);
            // The frame-buffers allocated from these pools will be automatically de-allocated...

            // Destroy synchronization objects
            vkDestroyFence(_device, _frames.at(i).renderFence, nullptr);
            vkDestroySemaphore(_device, _frames.at(i).swapchainImageAvailableSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames.at(i).renderFinishedSemaphore, nullptr);

            _frames.at(i).deletionQueue.flush();
        }
        // Flush the global deletion queue
        _mainDeletionQueue.flush();

        destroy_swapchain();
        vkDestroySurfaceKHR(_vulkanInstance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_vulkanInstance, _debugMessenger, nullptr);
        vkDestroyInstance(_vulkanInstance, nullptr);
        SDL_DestroyWindow(_window);
    }
    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw() {
    // Wait for the GPU to finish rendering the last frame (timeout of 1s)
    VkResult result = vkWaitForFences(_device, 1, &get_current_frame().renderFence, VK_TRUE, ENGINE_TIMEOUT_1_SECOND);
    if (result != VK_SUCCESS) {
        if (result == VK_TIMEOUT) {
            VK_LOG_WARN("VK_TIMEOUT - vkWaitForFences - Render Fence");
        }
        else {
            VK_LOG_ERROR("vkWaitForFences failed");
            throw std::runtime_error("vkWaitForFences failed");
        }
    }

    // Delete the resources of the current frame, since it's done rendering.
    get_current_frame().deletionQueue.flush();

    // Reset the render fence
    result = vkResetFences(_device, 1, &get_current_frame().renderFence);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkResetFences failed");
        throw std::runtime_error("vkResetFences failed");
    }

    // Request the index of an available image from the Swapchain (timeout of 1s)
    uint32_t swapchainImageIndex{};
    result = vkAcquireNextImageKHR(_device, _swapchain, ENGINE_TIMEOUT_1_SECOND, get_current_frame().swapchainImageAvailableSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    if (result != VK_SUCCESS) {
        if (result == VK_TIMEOUT) {
            VK_LOG_WARN("VK_TIMEOUT - vkAcquireNextImageKHR");
        }
        else if (result == VK_NOT_READY) {
            VK_LOG_ERROR("VK_NOT_READY - vkAcquireNextImageKHR");
        }
        else if (result == VK_SUBOPTIMAL_KHR) {
            VK_LOG_ERROR("VK_SUBOPTIMAL_KHR - vkAcquireNextImageKHR");
        }
    }

    // Reset the current frame's command-buffer
    VkCommandBuffer commandBuffer = get_current_frame().mainCommandBuffer;
    result = vkResetCommandBuffer(commandBuffer, 0);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkResetCommandBuffer failed");
        throw std::runtime_error("vkResetCommandBuffer failed");
    }

    // Begin the command buffer recording
    VkCommandBufferBeginInfo commandBufferBeginInfo {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkBeginCommandBuffer failed");
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    //
    // 1) Command buffer is now ready for recording commands onto it...
    // NEW CODE: We use the draw-image to render, and blit-copy it into the swapchain-image for presentation

    // Re-set the draw-extent (width and height) each frame
    _drawImageExtent.width = _drawImage.imageExtent.width;
    _drawImageExtent.height = _drawImage.imageExtent.height;

    // OPTIMIZED: Transition draw-image for writing into it
    // From undefined (don't care) to general (for clear operation)
    vkutil::transition_image_layout(
        commandBuffer,
        _drawImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,        // No previous work to wait for
        0,                                          // No previous access to sync
        VK_PIPELINE_STAGE_2_CLEAR_BIT,              // Wait for clear stage
        VK_ACCESS_2_TRANSFER_WRITE_BIT              // Clear operation writes
    );

    // Draw into the image using a compute-shader:
    VkImageSubresourceRange imageSubresourceRange {};
    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.baseMipLevel = 0;
    imageSubresourceRange.levelCount = 1;
    imageSubresourceRange.baseArrayLayer = 0;
    imageSubresourceRange.layerCount = 1;

    // Bind the pipeline for drawing with compute
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _backgroundImgPipeline);
    // Bind the descriptor sets
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _backgroundImgPipelineLayout, 0, 1, &_drawImageDescriptorSet, 0, nullptr);
    // Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it to get total group-counts needed along X and Y
    vkCmdDispatch(commandBuffer, std::ceil(_drawImageExtent.width / 16.0), std::ceil(_drawImageExtent.height / 16.0), 1);

    // OPTIMIZED: Transition draw-image for blit source
    // From general (after clear) to transfer source optimal
    vkutil::transition_image_layout(
        commandBuffer,
        _drawImage.image,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_2_CLEAR_BIT,            // Wait for clear to finish
        VK_ACCESS_2_TRANSFER_WRITE_BIT,           // Clear wrote to image
        VK_PIPELINE_STAGE_2_BLIT_BIT,             // Prepare for blit
        VK_ACCESS_2_TRANSFER_READ_BIT             // Blit will read from image
    );

    // OPTIMIZED: Transition swapchain image for blit destination
    // From undefined (don't care) to transfer destination optimal
    vkutil::transition_image_layout(
        commandBuffer,
        _swapchainImages.at(swapchainImageIndex),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,        // No previous work to wait for
        0,                                          // No previous access to sync
        VK_PIPELINE_STAGE_2_BLIT_BIT,               // Prepare for blit
        VK_ACCESS_2_TRANSFER_WRITE_BIT              // Blit will write to image
    );

    // Blit-copy from the draw-image to the swapchain-image to prepare it for presentation:
    vkutil::blit_image_to_image(
        commandBuffer,
        _drawImage.image,
        _swapchainImages.at(swapchainImageIndex),
        _drawImageExtent,
        _swapchainExtent
    );

    // OPTIMIZED: Transition swapchain image for presentation
    // From transfer destination (after blit) to present source
    vkutil::transition_image_layout(
        commandBuffer,
        _swapchainImages.at(swapchainImageIndex),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_BLIT_BIT,               // Wait for blit to finish
        VK_ACCESS_2_TRANSFER_WRITE_BIT,             // Blit wrote to image
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,     // No specific stage needs it after
        0                                           // No specific access needed
    );

    // Finish recording the command buffer
    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkEndCommandBuffer failed");
        throw std::runtime_error("vkEndCommandBuffer failed");
    }

    //
    // 2) We're now ready to submit the commands, along with the synchronization mechanisms to the queue...
    //

    // Prepare for submission to the queue:
    VkCommandBufferSubmitInfo commandBufferSubmitInfo {};
    commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferSubmitInfo.pNext = nullptr;
    commandBufferSubmitInfo.commandBuffer = commandBuffer;
    commandBufferSubmitInfo.deviceMask = 0;

    // OPTIMIZED: Wait for swapchain image availability at transfer stage
    VkSemaphoreSubmitInfo waitSemaphoreInfo {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.pNext = nullptr;
    waitSemaphoreInfo.deviceIndex = 0;
    waitSemaphoreInfo.value = 1;
    waitSemaphoreInfo.semaphore = get_current_frame().swapchainImageAvailableSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;  // Wait for blit operations

    // OPTIMIZED: Signal when all transfer operations complete
    VkSemaphoreSubmitInfo signalSemaphoreInfo {};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.pNext = nullptr;
    signalSemaphoreInfo.deviceIndex = 0;
    signalSemaphoreInfo.value = 1;
    signalSemaphoreInfo.semaphore = get_current_frame().renderFinishedSemaphore;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;  // Signal after blit completes

    // Pass all the submission info to VkSubmitInfo2 struct
    VkSubmitInfo2 cmdSubmitInfo{};
    cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    cmdSubmitInfo.pNext = nullptr;
    cmdSubmitInfo.commandBufferInfoCount = 1;
    cmdSubmitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
    cmdSubmitInfo.waitSemaphoreInfoCount = 1;
    cmdSubmitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    cmdSubmitInfo.signalSemaphoreInfoCount = 1;
    cmdSubmitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    // Submit the command buffer to the queue for execution:
    // render-fence will now be blocked until these graphics commands finish execution
    result = vkQueueSubmit2(_graphicsQueue, 1, &cmdSubmitInfo, get_current_frame().renderFence);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkQueueSubmit2 failed");
        throw std::runtime_error("vkQueueSubmit2 failed");
    }

    //
    // 3) We now present the image that finished rendering in the previous step...
    //

    // Prepare for presentation:
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &swapchainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame().renderFinishedSemaphore;

    result = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("vkQueuePresentKHR failed");
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    // Increment the frame number drawn:
    ++_frameNumber;

    // Ensures that the presentation engine is done presenting the image, before proceeding...
    vkQueueWaitIdle(_graphicsQueue);
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_EVENT_QUIT)  // SDL3: SDL_EVENT_QUIT instead of SDL_QUIT
                bQuit = true;

            // SDL3: Window events are now SDL_EVENT_WINDOW_*
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                stop_rendering = false;
            }

            // SDL3: Key events are now SDL_EVENT_KEY_DOWN
            if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.scancode) {
                    // If ESCAPE key was pressed, quit the application
                    case SDL_SCANCODE_ESCAPE:
                        VK_LOG_INFO("ESCAPE - Exiting application");
                        bQuit = true;
                    default: break;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}


/// @brief Initializes the core Vulkan components using vk-bootstrap library.
///
/// This function sets up the complete Vulkan foundation required for rendering:
/// \n - Creates Vulkan instance with validation layers and debug messenger
/// \n - Establishes surface connection to the SDL window
/// \n - Selects physical device with required features (Vulkan 1.3 and 1.2)
/// \n - Creates logical device
///
/// Required Vulkan 1.3 features:
/// \n - Dynamic rendering pipeline
/// \n - Synchronization2 for improved sync primitives
///
/// Required Vulkan 1.2 features:
/// \n - Buffer device address for GPU-side buffer references
/// \n - Descriptor indexing for bindless resource access
///
/// @attention Requires SDL window (_window) to be created before calling
/// @throws std::runtime_error if any Vulkan component fails to initialize
/// @note Sets internal handles: _vulkanInstance, _debugMessenger, _surface, _physicalDevice, _device, _graphicsQueue
void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder instanceBuilder;
    // Create the Vulkan instance with basic debug features
    auto instance_ret = instanceBuilder
        .set_app_name("Vulkan Engine")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();
    vkb::Instance vkb_instance = instance_ret.value();

    // Set the instance
    _vulkanInstance = vkb_instance;
    _debugMessenger = vkb_instance.debug_messenger;

    // Set the handle to the surface from the SDL window
    SDL_Vulkan_CreateSurface(_window, _vulkanInstance, nullptr, &_surface);

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features vulkan13_features{};
    vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13_features.dynamicRendering = true;
    vulkan13_features.synchronization2 = true;

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features vulkan12_features{};
    vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12_features.bufferDeviceAddress = true;
    vulkan12_features.descriptorIndexing = true;

    // Use vk-bootstrap to select a suitable GPU (physical device)
    vkb::PhysicalDeviceSelector physicalDeviceSelector {vkb_instance};
    vkb::PhysicalDevice vkb_physical_device = physicalDeviceSelector
        .set_minimum_version(1, 3)
        .set_required_features_13(vulkan13_features)
        .set_required_features_12(vulkan12_features)
        .set_surface(_surface)
        .select()
        .value();

    // Create the final Vulkan device (logical device)
    vkb::DeviceBuilder deviceBuilder {vkb_physical_device};
    vkb::Device vkb_device = deviceBuilder.build().value();

    // Set the handles to the physical and logical device
    _physicalDevice = vkb_physical_device.physical_device;
    _device = vkb_device.device;

    // Store the handle to a graphics-queue and its queue family index
    _graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize VMA allocator
    init_vulkan_memory_allocator();
}

void VulkanEngine::init_swapchain() {
    // Create the swapchain
    create_swapchain(_windowExtent.width, _windowExtent.height);

    // Allocate the image that we will be drawing into, in our draw loop:
    // The image size of the drawing-image will match the window size
    VkExtent3D drawImageExtent {_windowExtent.width, _windowExtent.height, 1};

    // Hardcoding the draw format to 16-bit float (rgba)
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    // Specify the usages of the draw-image
    VkImageUsageFlags drawImageUsageFlags{};
    drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // We're now ready to allocate the draw-image...
    VkImageCreateInfo drawImageCreateInfo{};
    drawImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    drawImageCreateInfo.pNext = nullptr;
    drawImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    drawImageCreateInfo.format = _drawImage.imageFormat;
    drawImageCreateInfo.extent = _drawImage.imageExtent;
    drawImageCreateInfo.mipLevels = 1;
    drawImageCreateInfo.arrayLayers = 1;
    drawImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // For MSAA. Will not be using it now.
    drawImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    drawImageCreateInfo.usage = drawImageUsageFlags;
    // Specify the allocation of the draw-image to be from GPU-local memory using the VMA allocator
    VmaAllocationCreateInfo drawImageAllocationCreateInfo{};
    drawImageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // Allocate and create the image
    VkResult result = vmaCreateImage(_vmaAllocator, &drawImageCreateInfo, &drawImageAllocationCreateInfo, &_drawImage.image, &_drawImage.vmaAllocation, nullptr);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create draw image!");
        throw std::runtime_error("Failed to create draw image!");
    }
    VK_LOG_SUCCESS("Draw image created");

    // Build an image-view of the draw-image to use for rendering:
    VkImageViewCreateInfo drawImageViewCreateInfo{};
    drawImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    drawImageViewCreateInfo.pNext = nullptr;
    drawImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    drawImageViewCreateInfo.image = _drawImage.image;
    drawImageViewCreateInfo.format = _drawImage.imageFormat;
    drawImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    drawImageViewCreateInfo.subresourceRange.levelCount = 1;
    drawImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    drawImageViewCreateInfo.subresourceRange.layerCount = 1;
    drawImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // Create the image-view:
    result = vkCreateImageView(_device, &drawImageViewCreateInfo, nullptr, &_drawImage.imageView);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create draw image-view!");
        throw std::runtime_error("Failed to create draw image-view!");
    }
    VK_LOG_SUCCESS("Draw image-view created");


    // Add to main deletion queue:
    _mainDeletionQueue.push_deleter([&]() {
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_vmaAllocator, _drawImage.image, _drawImage.vmaAllocation);
    });
}

void VulkanEngine::init_commands() {
    // Have the command-pool allow resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = _graphicsQueueFamilyIndex;

    for (int i{0}; i < FRAME_OVERLAP; i++) {
        VkResult result = vkCreateCommandPool(_device, &command_pool_create_info, nullptr, &_frames.at(i).commandPool);
        if (result != VK_SUCCESS) {
            VK_LOG_ERROR("Failed to create command pool");
            throw std::runtime_error("Failed to create command pool");
        }

        // Allocate the default command buffer for this frame, that will be used for rendering
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = nullptr;
        command_buffer_allocate_info.commandPool = _frames.at(i).commandPool;
        command_buffer_allocate_info.commandBufferCount = 1;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        result = vkAllocateCommandBuffers(_device, &command_buffer_allocate_info, &_frames.at(i).mainCommandBuffer);
        if (result != VK_SUCCESS) {
            VK_LOG_ERROR("Failed to create command buffer");
            throw std::runtime_error("Failed to create command buffer");
        }
    }

}

void VulkanEngine::init_sync_structures() {
    for (int i{0}; i < FRAME_OVERLAP; i++) {
        // We want the Fence to start in the signalled state, so we can wait on it on the first frame.
        VkFenceCreateInfo renderFenceCreateInfo{};
        renderFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        renderFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Create the Fence:
        VkResult result = vkCreateFence(_device, &renderFenceCreateInfo, nullptr, &_frames.at(i).renderFence);
        if (result != VK_SUCCESS) {
            VK_LOG_ERROR("Failed to create render fence");
            throw std::runtime_error("Failed to create render fence");
        }

        // Create the Semaphores:
        result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames.at(i).swapchainImageAvailableSemaphore);
        if (result != VK_SUCCESS) {
            VK_LOG_ERROR("Failed to create swapchain image available semaphore");
            throw std::runtime_error("Failed to create swapchain image available semaphore");
        }

        result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames.at(i).renderFinishedSemaphore);
        if (result != VK_SUCCESS) {
            VK_LOG_ERROR("Failed to create render finished semaphore");
            throw std::runtime_error("Failed to create render finished semaphore");
        }
    }
}

void VulkanEngine::init_pipelines() {
    init_background_img_pipeline();
}

void VulkanEngine::init_vulkan_memory_allocator() {
    VmaAllocatorCreateInfo allocator_create_info{};
    allocator_create_info.instance = _vulkanInstance;
    allocator_create_info.physicalDevice = _physicalDevice;
    allocator_create_info.device = _device;
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // Lets use GPU pointers

    VkResult result = vmaCreateAllocator(&allocator_create_info, &_vmaAllocator);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create VMA allocator");
        throw std::runtime_error("Failed to create VMA allocator");
    }
    VK_LOG_SUCCESS("Created VMA allocator");

    _mainDeletionQueue.push_deleter([&]() {
        vmaDestroyAllocator(_vmaAllocator);
    });
}

void VulkanEngine::init_descriptors() {
    // We'll create a descriptor-pool that will hold 10 sets of 1 descriptor each of type storage-image
    std::vector<DescriptorSetAllocator::PoolSizeRatio> sizeRatios = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    // Max 10 Descriptor-Sets
    // 10 Descriptors - all of them of type VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    _globalDescriptorSetAllocator.init_descriptor_pool(_device, 10, sizeRatios);

    // Make the descriptor-set-layout for the compute-draw
    // Each Descriptor-Set will only have 1 Descriptor of type VK_DESCRIPTOR_TYPE_STORAGE_IMAGE at binding 0
    {
        DescriptorLayoutBuilder layoutBuilder{};
        layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorSetLayout = layoutBuilder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }


    // Allocate 1 descriptor-set for our draw-image:
    _drawImageDescriptorSet = _globalDescriptorSetAllocator.allocate_descriptor_set(_device, _drawImageDescriptorSetLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = _drawImage.imageView;

    VkWriteDescriptorSet drawImageWrite{};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;
    drawImageWrite.dstSet = _drawImageDescriptorSet;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.dstBinding = 0;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imageInfo;

    // Update the draw-image's descriptor-set with the info. regarding the image
    vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

    // Ensuring that the global descriptor-set allocator and the layout gets cleaned up
    _mainDeletionQueue.push_deleter([&]() {
        _globalDescriptorSetAllocator.destroy_descriptor_pool(_device);
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorSetLayout, nullptr);
    });
}

void VulkanEngine::init_background_img_pipeline() {
    // Create the Pipeline-Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.pSetLayouts = &_drawImageDescriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    VkResult result = vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &_backgroundImgPipelineLayout);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create pipeline-layout for background-img draw");
        throw std::runtime_error("Failed to create pipeline-layout for background-img draw");
    }
    VK_LOG_SUCCESS("Created pipeline-layout for background-img draw");

    // Create the Compute-Pipeline
    // Load the SpirV compiled compute-shader
    VkShaderModule computeDrawShaderModule;
    if (!vkutil::load_shader_module(_device, &computeDrawShaderModule, "./shaders/gradient.comp.spv")) {
        VK_LOG_ERROR("Failed to load SpirV shader: gradient.comp.spv");
        throw std::runtime_error("Failed to load SpirV shader: gradient.comp.spv");
    }
    VK_LOG_INFO("Loaded SpirV shader: gradient.comp.spv");
    VK_LOG_INFO("Created compute shader-module from shader: gradient.comp.spv");

    VkPipelineShaderStageCreateInfo pipelineShaderStageInfo{};
    pipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageInfo.pNext = nullptr;
    pipelineShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineShaderStageInfo.module = computeDrawShaderModule;
    pipelineShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = _backgroundImgPipelineLayout;
    computePipelineCreateInfo.stage = pipelineShaderStageInfo;

    result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_backgroundImgPipeline);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create compute pipeline");
        throw std::runtime_error("Failed to create compute pipeline");
    }
    VK_LOG_SUCCESS("Created compute pipeline");


    // Schedule cleanup
    vkDestroyShaderModule(_device, computeDrawShaderModule, nullptr);  // We no longer need it after creating the pipeline
    _mainDeletionQueue.push_deleter([&]() {
        vkDestroyPipelineLayout(_device, _backgroundImgPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _backgroundImgPipeline, nullptr);
    });
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder {_physicalDevice, _device, _surface};

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR {.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchainExtent = vkbSwapchain.extent;
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    // Destroying the swapchain will delete the images it holds internally.
    for (size_t i{0}; i < _swapchainImageViews.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews.at(i), nullptr);
    }
}