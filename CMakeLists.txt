cmake_minimum_required(VERSION 3.20)
project(VulkanEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.5")

# Create executable
add_executable(VulkanEngine src/main.cpp)

# We'll add libraries step by step