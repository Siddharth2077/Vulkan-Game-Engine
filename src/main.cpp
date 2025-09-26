#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/format.h>

int main() {
    fmt::print("Hello from CLion Vulkan Engine!\n");

    // Test GLM
    glm::vec3 position(1.0f, 2.0f, 3.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    // Test fmt with GLM
    fmt::print("GLM + fmt test successful!\n");
    fmt::print("Position vector: ({}, {}, {})\n", position.x, position.y, position.z);

    // Test some fmt formatting features
    fmt::print("Formatted number: {:.2f}\n", 3.14159);
    fmt::print("Hex value: 0x{:04X}\n", 255);

    return 0;
}