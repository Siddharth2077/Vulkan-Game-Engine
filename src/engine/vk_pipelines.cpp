#include "vk_pipelines.h"
#include <fstream>
#include "vk_logger.h"


bool vkutil::load_shader_module(VkDevice device, VkShaderModule*outShaderModule, const char *filePath) {
    // Open the file, with the cursor at the end
    std::ifstream shaderFile(filePath, std::ios::binary | std::ios::ate);
    if (!shaderFile.is_open()) {
        VK_LOG_WARN("Failed to open file %s", filePath);
        return false;
    }

    // Find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t shaderFileSize = (size_t)shaderFile.tellg();

    // SpirV expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer (shaderFileSize / sizeof(uint32_t));

    // Place file cursor at beginning of file
    shaderFile.seekg(0);

    // load the entire file into the buffer
    shaderFile.read(reinterpret_cast<char *>(buffer.data()), shaderFileSize);

    // now that the file is loaded into the buffer, we can close it
    shaderFile.close();


    // Create a new Shader-Module from the shader that we loaded
    VkShaderModuleCreateInfo shaderModuleCreateInfo {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);  // code-size in bytes
    shaderModuleCreateInfo.pCode = buffer.data();

    VkShaderModule shaderModule {VK_NULL_HANDLE};
    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        VK_LOG_WARN("Failed to create shader module!");
        return false;
    }
    VK_LOG_SUCCESS("Created shader module");

    *outShaderModule = shaderModule;
    return true;
}
