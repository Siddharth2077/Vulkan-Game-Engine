#pragma once 
#include "vk_types.h"

namespace vkutil {

    bool load_shader_module(VkDevice device, VkShaderModule* outShaderModule, const char* filePath);

};