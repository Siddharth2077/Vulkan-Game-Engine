#include <fmt/format.h>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

int main() {
    // Test fmt
    fmt::print("Hello from fmt!\n");

    // Test GLM
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    fmt::print("GLM vector: ({}, {}, {})\n", testVec.x, testVec.y, testVec.z);

    // Test SDL3
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fmt::print("SDL3 initialization failed: {}\n", SDL_GetError());
        return 1;
    }
    fmt::print("SDL3 initialized successfully!\n");

    // Test vk-bootstrap
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("VulkanEngine Test")
                          .request_validation_layers(true)
                          .require_api_version(1, 1, 0)
                          .build();

    if (!inst_ret) {
        fmt::print("Failed to create Vulkan instance: {}\n", inst_ret.error().message());
        SDL_Quit();
        return 1;
    }

    fmt::print("vk-bootstrap: Vulkan instance created successfully!\n");
    vkb::Instance vkb_inst = inst_ret.value();

    // Test VMA
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    fmt::print("VMA: Successfully included VulkanMemoryAllocator!\n");

    // Test STB
    fmt::print("STB: stb_image version {}\n", STBI_VERSION);

    // Test ImGui
    fmt::print("ImGui: Successfully included ImGui version {}!\n", IMGUI_VERSION);

    // Clean up
    vkb::destroy_instance(vkb_inst);
    SDL_Quit();
    return 0;
}