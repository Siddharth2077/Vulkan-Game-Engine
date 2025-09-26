#include <fmt/format.h>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <VkBootstrap.h>

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
    } else {
        fmt::print("vk-bootstrap: Vulkan instance created successfully!\n");

        vkb::Instance vkb_inst = inst_ret.value();

        // Clean up
        destroy_instance(vkb_inst);
    }

    SDL_Quit();
    return 0;
}