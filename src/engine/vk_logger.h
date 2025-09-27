#pragma once

#include <fmt/core.h>
#include <fmt/color.h>

#ifndef NDEBUG
    #define VK_LOG_INFO(msg, ...) fmt::print(fmt::fg(fmt::color::gray),    "[INFO]    " msg "\n", ##__VA_ARGS__)
    #define VK_LOG_WARN(msg, ...) fmt::print(fmt::fg(fmt::color::yellow),   "[WARN]    " msg "\n", ##__VA_ARGS__)
    #define VK_LOG_ERROR(msg, ...) fmt::print(fmt::fg(fmt::color::red),     "[ERROR]   " msg "\n", ##__VA_ARGS__)
    #define VK_LOG_DEBUG(msg, ...) fmt::print(fmt::fg(fmt::color::cyan),    "[DEBUG]   " msg "\n", ##__VA_ARGS__)
    #define VK_LOG_SUCCESS(msg, ...) fmt::print(fmt::fg(fmt::color::green), "[SUCCESS] " msg "\n", ##__VA_ARGS__)
#else
    #define VK_LOG_INFO(msg, ...)
    #define VK_LOG_WARN(msg, ...)
    #define VK_LOG_ERROR(msg, ...)
    #define VK_LOG_DEBUG(msg, ...)
#endif
