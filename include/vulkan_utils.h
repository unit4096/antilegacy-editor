#pragma once

// ext
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif //GLM


// Int
#include <primitives.h>

namespace ale {
namespace vk {

VkPipelineShaderStageCreateInfo getShaderStageInfo(VkShaderModule &module, VkShaderStageFlagBits stage, std::string name);

VkPipelineShaderStageCreateInfo getShaderStageInfo(
                                                    VkShaderModule &module,
                                                    VkShaderStageFlagBits stage,
                                                    std::string name) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = name.data(),
    };
}


    
} // namespace vk
} // namespace ale



