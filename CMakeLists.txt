cmake_minimum_required(VERSION 3.20)
project(VulkanEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.5")

# Find Vulkan
find_package(Vulkan REQUIRED)

# Third-party directory
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

# GLM (header-only math library)
set(GLM_INCLUDE_DIR "${THIRD_PARTY_DIR}/glm")

# VMA (header-only memory allocator)
set(VMA_INCLUDE_DIR "${THIRD_PARTY_DIR}/VulkanMemoryAllocator/include")

# STB (header-only image/utility libraries)
set(STB_INCLUDE_DIR "${THIRD_PARTY_DIR}/stb")

# ImGui setup
set(IMGUI_DIR "${THIRD_PARTY_DIR}/imgui")
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Add fmt library (needs compilation)
add_subdirectory(${THIRD_PARTY_DIR}/fmt)

# Add SDL3 library
add_subdirectory(${THIRD_PARTY_DIR}/SDL3)

# Add vk-bootstrap library
add_subdirectory(${THIRD_PARTY_DIR}/vk-bootstrap)

# Add fastgltf library
add_subdirectory(${THIRD_PARTY_DIR}/fastgltf)

# Create executable
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
add_executable(VulkanEngine ${SOURCES} ${IMGUI_SOURCES})

# Include directories
target_include_directories(VulkanEngine PRIVATE
        ${GLM_INCLUDE_DIR}
        ${VMA_INCLUDE_DIR}
        ${STB_INCLUDE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(VulkanEngine
        fmt::fmt
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap
        fastgltf
        ${Vulkan_LIBRARIES}
)