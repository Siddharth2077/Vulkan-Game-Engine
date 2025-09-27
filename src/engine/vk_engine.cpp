#include "vk_engine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "vk_initializers.h"
#include "vk_types.h"
#include <chrono>
#include <thread>
#include <VkBootstrap.h>
#include "vk_logger.h"

constexpr bool bUseValidationLayers {true};

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

    // Everything went fine
    _isInitialized = true;
    VK_LOG_SUCCESS("Initialized VulkanEngine");
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
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
    // nothing yet
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
                // SDL3: Use e.key.scancode directly (no .keysym needed)
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
    auto instance_ret = instanceBuilder.set_app_name("Vulkan Engine")
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
}

void VulkanEngine::init_swapchain() {
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands() {
    // TODO
}

void VulkanEngine::init_sync_structures() {
    // TODO
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
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}