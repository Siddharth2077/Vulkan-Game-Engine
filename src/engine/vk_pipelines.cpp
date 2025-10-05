#include "vk_pipelines.h"
#include <fstream>

#include "VkBootstrap.h"
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


// GraphicsPipelineBuilder method definitions:

void GraphicsPipelineBuilder::clear() {
    _inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    _rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    _colorBlendAttachment = {};
    _multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    _depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    _dynamicRenderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    _pipelineLayout = {};
    _shaderStages.clear();
}

void GraphicsPipelineBuilder::set_pipeline_layout(VkPipelineLayout pipelineLayout) {
    _pipelineLayout = pipelineLayout;
}

void GraphicsPipelineBuilder::set_shader_modules(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule) {
    _shaderStages.clear();

    // Push the vertex-shader to the _shaderStages array:
    VkPipelineShaderStageCreateInfo vertex_shader_stage {};
    vertex_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage.pNext = nullptr;
    vertex_shader_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage.module = vertexShaderModule;
    vertex_shader_stage.pName = "main";
    _shaderStages.push_back(vertex_shader_stage);

    // Push the fragment-shader to the _shaderStages array:
    VkPipelineShaderStageCreateInfo fragment_shader_stage {};
    fragment_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage.pNext = nullptr;
    fragment_shader_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage.module = fragmentShaderModule;
    fragment_shader_stage.pName = "main";
    _shaderStages.push_back(fragment_shader_stage);
}

void GraphicsPipelineBuilder::set_input_topology(VkPrimitiveTopology topology) {
    _inputAssembly.topology = topology;

    // We won't be using primitive-restart
    // Used for triangle-strips and line-strips
    _inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::set_polygon_mode(VkPolygonMode polygonMode, float lineWidth) {
    _rasterizer.polygonMode = polygonMode;
    _rasterizer.lineWidth = lineWidth;  // defaulted to 1.0f
}

void GraphicsPipelineBuilder::set_cull_mode(VkCullModeFlags cullModeFlags, VkFrontFace frontFace) {
    _rasterizer.cullMode = cullModeFlags;
    _rasterizer.frontFace = frontFace;
}

void GraphicsPipelineBuilder::set_multisampling_none() {
    _multisampling.sampleShadingEnable = VK_FALSE;
    // No multisampling (1 sample per pixel)
    _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _multisampling.minSampleShading = 1.0f;
    _multisampling.pSampleMask = nullptr;
    // No alpha to coverage
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::set_blending_none() {
    // Default write mask
    _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // No blending
    _colorBlendAttachment.blendEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::set_color_attachment_format(VkFormat colorAttachmentFormat) {
    _colorAttachmentFormat = colorAttachmentFormat;
    // Connect the format to the render-info struct
    _dynamicRenderInfo.colorAttachmentCount = 1;
    _dynamicRenderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void GraphicsPipelineBuilder::set_depth_attachment_format(VkFormat depthAttachmentFormat) {
    _dynamicRenderInfo.depthAttachmentFormat = depthAttachmentFormat;
}

void GraphicsPipelineBuilder::disable_depth_testing() {
    _depthStencil.depthTestEnable = VK_FALSE;
    _depthStencil.depthWriteEnable = VK_FALSE;
    _depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    _depthStencil.depthBoundsTestEnable = VK_FALSE;
    _depthStencil.stencilTestEnable = VK_FALSE;
    _depthStencil.front = {};
    _depthStencil.back = {};
    _depthStencil.minDepthBounds = 0.f;
    _depthStencil.maxDepthBounds = 1.f;
}

VkPipeline GraphicsPipelineBuilder::build_pipeline(VkDevice device) {
    // Make the Viewport state (will only support one viewport and scissor currently)
    // Viewport and Scissor will be dynamic, hence they'll be set during command-buffer recording time
    VkPipelineViewportStateCreateInfo viewport_state_create_info {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.pNext = nullptr;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    // Setup dummy color blending (not using transparent objects yet)
    // This blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext = nullptr;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;  // dummy
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &_colorBlendAttachment;

    // Completely clear the VertexInputStateCreateInfo (currently no need for it since we're "vertex-pulling")
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};


    // Build the actual Graphics-Pipeline:
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // Connect the _dynamicRenderInfo struct into the pNext chain, since we'll be using dynamic-rendering instead of renderpassers
    graphics_pipeline_create_info.pNext = &_dynamicRenderInfo;
    graphics_pipeline_create_info.stageCount = static_cast<uint32_t>(_shaderStages.size());
    graphics_pipeline_create_info.pStages = _shaderStages.data();
    graphics_pipeline_create_info.pVertexInputState = &_vertexInputInfo;
    graphics_pipeline_create_info.pInputAssemblyState = &_inputAssembly;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &_rasterizer;
    graphics_pipeline_create_info.pMultisampleState = &_multisampling;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &_depthStencil;
    graphics_pipeline_create_info.layout = _pipelineLayout;

    // Define the dynamic states
    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = nullptr;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;


    // Create the Graphics-Pipeline
    VkPipeline newGraphicsPipeline {VK_NULL_HANDLE};
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &newGraphicsPipeline);
    if (result != VK_SUCCESS) {
        VK_LOG_ERROR("Failed to create graphics-pipeline");
        throw std::runtime_error("Failed to create graphics-pipeline");
    }
    VK_LOG_SUCCESS("Created graphics-pipeline");

    return newGraphicsPipeline;
}
