#include <fmt/format.h>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

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

    SDL_Quit();
    return 0;
}