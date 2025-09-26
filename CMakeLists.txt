cmake_minimum_required(VERSION 3.20)
project(VulkanEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.5")

# Third-party directory
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

# GLM (header-only math library)
set(GLM_INCLUDE_DIR "${THIRD_PARTY_DIR}/glm")

# Create executable
add_executable(VulkanEngine src/main.cpp)

# Include directories
target_include_directories(VulkanEngine PRIVATE
        ${GLM_INCLUDE_DIR}
)