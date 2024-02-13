#pragma once

/* 
    Utility functions for Vulkan. 
*/
// ext
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#endif //GLM

#ifndef ALE_VK_UTILS
#define ALE_VK_UTILS

// Int
#include <primitives.h>

namespace ale {
namespace vk {


// ================
// Getters for vulkan info structs
// ================
// {{{

// ----
// Descriptor set arguments
// ----

static void pushBackDescriptorSetBinding(
                            std::vector<VkDescriptorSetLayoutBinding>& bindings,
                            const unsigned int count,
                            const VkDescriptorType type,
                            const VkShaderStageFlags flags,
                            const VkSampler* immutableSamplers = nullptr) {

    VkDescriptorSetLayoutBinding binding {};
    binding.binding = bindings.size();
    binding.descriptorCount = count;
    binding.descriptorType = type;
    binding.stageFlags = flags;
    binding.pImmutableSamplers = immutableSamplers;

    bindings.push_back(binding);
}

// ----
// Vulkan pipeline creation
// ----


static VkPipelineShaderStageCreateInfo getShaderStageInfo(
										VkShaderModule &module,
                                        VkShaderStageFlagBits stage,
                                        std::string& name) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = name.data(),
    };
}


// Create a VK shader module from raw .spv code for a given device
static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}


// Creates a binding description based on binding location and its stride in bytes
static VkVertexInputBindingDescription getVertBindingDescription(
                                                        unsigned int binding,
                                                        unsigned int stride) {    
    return {
        .binding = binding,
        .stride = stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
}


static VkPipelineVertexInputStateCreateInfo getVertexInputInfo(
                const VkVertexInputBindingDescription& bindingDescription,
                const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
};


static VkPipelineDynamicStateCreateInfo getPipelineDynamicState(
                            const std::vector<VkDynamicState>& dynamicStates) {

    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

};


static VkPipelineLayoutCreateInfo getPipelineLayout(
                const VkDescriptorSetLayout& descriptorSetLayout,
                const std::vector<VkPushConstantRange>& pushConstantRanges) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = static_cast<unsigned int>(pushConstantRanges.size()),
        .pPushConstantRanges = &pushConstantRanges.at(0),
    };
};


static VkPipelineColorBlendStateCreateInfo getColorBlending(
            const VkPipelineColorBlendAttachmentState& colorBlendAttachment) {

    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0,0,0,0},
    };
    
};


// ----
// Image creation
// ----

static VkImageCreateInfo getImageInfo(unsigned int w, unsigned int h,
                                VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage) {

    VkExtent3D extent = {
        .width = w,
        .height = h,
        .depth = 1,
    };

    return {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
};


// ----
// Draw command 
// ----

static VkRenderPassBeginInfo getRenderPassBegin(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D swapChainExtent) {

    VkRect2D renderArea {
        .offset = {0, 0},
        .extent = swapChainExtent,
    };
    
    return {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = VK_NULL_HANDLE,
        .renderPass = renderPass,
        .framebuffer = frameBuffer,
        .renderArea = renderArea,
    };
};

static VkViewport getViewport(VkExtent2D swapChainExtent){
    return {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) swapChainExtent.width,
        .height = (float) swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
};


// }}}


// ================
// Getters for "default" vulkan structs. Exist to shrink boilerplate code
// ================

// {{{

static VkDescriptorSetLayoutCreateInfo getDescriptorSetLayout(
                    const std::vector<VkDescriptorSetLayoutBinding>& bindings){
    return {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

};

// ----
// Render pipeline arguments
// ----

static VkPipelineViewportStateCreateInfo getDefaultViewportState(){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
};


static VkPipelineInputAssemblyStateCreateInfo getDefaultInputAssembly() {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
};


static VkPipelineRasterizationStateCreateInfo  getDefaultRasterizer() {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };
};


static VkPipelineMultisampleStateCreateInfo getDefaultMultisampling() {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };
};
    

static VkPipelineDepthStencilStateCreateInfo getDefaultDepthStencil() {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
};

// }}}
    


} // namespace vk
} // namespace ale

#endif //ALE_VK_UTILS
