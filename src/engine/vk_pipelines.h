#pragma once 
#include "vk_types.h"

namespace vkutil {
    bool load_shader_module(VkDevice device, VkShaderModule* outShaderModule, const char* filePath);
};


/// Graphics-Pipeline Builder class
class GraphicsPipelineBuilder {
public:
    /// Class-Constructor
    GraphicsPipelineBuilder() {clear();}

    /// Clears all the Vulkan structs back to 0, with their correct sType value
    void clear();

    void set_pipeline_layout(VkPipelineLayout pipelineLayout);
    void set_shader_modules(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode polygonMode, float lineWidth = 1.0f);
    void set_cull_mode(VkCullModeFlags cullModeFlags, VkFrontFace frontFace);
    void set_multisampling_none();
    void set_blending_none();
    void set_color_attachment_format(VkFormat colorAttachmentFormat);
    void set_depth_attachment_format(VkFormat depthAttachmentFormat);
    void disable_depth_testing();

    VkPipeline build_pipeline(VkDevice device);

private:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _dynamicRenderInfo;

    VkFormat _colorAttachmentFormat;
    VkPipelineLayout _pipelineLayout;

};