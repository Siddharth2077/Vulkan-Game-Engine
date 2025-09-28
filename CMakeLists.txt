cmake_minimum_required(VERSION 3.20)
project(VulkanEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.5")

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find glslc compiler
find_program(GLSLC_EXECUTABLE glslc HINTS
        ${Vulkan_GLSLC_EXECUTABLE}
        REQUIRED
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed.")
else()
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
endif()


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


# Shader compilation setup
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Find all shader files
file(GLOB_RECURSE SHADER_FILES
        "${SHADER_SOURCE_DIR}/*.vert"
        "${SHADER_SOURCE_DIR}/*.frag"
        "${SHADER_SOURCE_DIR}/*.comp"
)

# Function to compile a single shader
function(compile_shader SHADER_SOURCE SHADER_OUTPUT)
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader: ${SHADER_SOURCE}"
            VERBATIM
    )
endfunction()

# Compile all shaders
set(COMPILED_SHADERS)
foreach(SHADER_FILE ${SHADER_FILES})
    # Get relative path from shader source directory
    file(RELATIVE_PATH SHADER_RELATIVE ${SHADER_SOURCE_DIR} ${SHADER_FILE})

    # Create output path with .spv extension
    set(SHADER_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_RELATIVE}.spv")

    # Get the directory of the output file and ensure it exists
    get_filename_component(SHADER_OUTPUT_DIR ${SHADER_OUTPUT} DIRECTORY)

    # Compile this shader
    compile_shader(${SHADER_FILE} ${SHADER_OUTPUT})

    # Add to list of compiled shaders
    list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})
endforeach()

# Create a custom target for all shaders
add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})


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

# Print shader info for debugging
list(LENGTH SHADER_FILES SHADER_COUNT)
if(SHADER_COUNT GREATER 0)
    message(STATUS "Found ${SHADER_COUNT} shader files:")
    foreach(SHADER ${SHADER_FILES})
        message(STATUS "  ${SHADER}")
    endforeach()
else()
    message(STATUS "No shader files found in ${SHADER_SOURCE_DIR}")
endif()