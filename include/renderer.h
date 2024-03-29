/*

    Main renderer for Antilegacy Editor. Uses Vulkan API to render
    scenes in ale::Model format.

    As of 13.03.2024 it is Work In Progress and currently requires
    a material system overhaul

Code structure details:
- The renderer has a structure of a single header file to reduce compilation
  time during development iterations. In the future this code will be
  moved to a separate precompiled module (but clangd support is not here yet)
- Most of the inner headers are included using angled <...> includes.
  This allows for quick project restructuring during prototyping.
  Internal and external dependencies are separate by // ext and // int
  comments

Implementation details:
- boilerplate code from the renderer was taken from vulkan-tutorial.com.
  Thus, it inherited some naming conventions from this code.
  POI: initVulkan() keeps original structure
- The renderer features a classic vertex shader to fragment shader pipeline
  using dynamic rendering.
- The renderer uses GLTF for window management.
  POI: initWindow(), glfwInit()
- This code uses VkBootstrap to reduce boilerplate line count. Additionaly,
  some imitialization functions are moved to vulkan_utils.h
  POI: look for `vkb_` and vkb:: namespaces prefixes for VkBootstrap
  structures
- This code uses a single buffer for all vertices and a single buffer for
  all indices for performance reasons. It uses push constants to
  apply individual object positions
  POI: createVertexBuffer(), createIndexBuffer(), pushConstantRanges array

Upcoming features:
- Optimized rendering for multiple-material scenes
- Runtime model streaming and shader switching for 3D editing
- Accurate PBR rendering
*/

#ifndef ALE_RENDERER
#define ALE_RENDERER

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


#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
#include <stack>


// int
#include <vkbootstrap/VkBootstrap.h>

#include <primitives.h>
#include <camera.h>
#include <tracer.h>
#include <vulkan_utils.h>
#include <os_loader.h>
#include <memory.h>
#include <ale_imgui_interface.h>
#include <ui_manager.h>


namespace trc = ale::Tracer;

namespace ale {

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;

const int MAX_FRAMES_IN_FLIGHT = 3;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME,
};

#ifdef NDEBUG
const bool renderer_enableValidationLayers = false;
#else
const bool renderer_enableValidationLayers = true;
#endif


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(4) glm::vec4 light;
};

struct Line2D {
    alignas(8) glm::vec2 start;
    alignas(8) glm::vec2 end;
};

struct MeshBufferData{
    unsigned int size;
    unsigned int offset;
    glm::mat4 transform;
};


struct PushConstantData {
    unsigned int offset;
    unsigned int size;
    VkShaderStageFlagBits stages;
};


class Renderer {
public:

    Renderer(ale::Model& _model):model(_model){
        // FIXME: For now, treats the first texture as the default tex for all meshes.
        image = model.textures[0];
    };


    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Antilegacy Editor", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        destructorStack.push([this](){
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        });
    }

    void initRenderer() {
        initVulkan();
        initImGUI();
    }


    void bindCamera(ale::sp<Camera> cam) {
        this->mainCamera = cam;
    }

    sp<ale::Camera> getCurrentCamera() {
        return mainCamera;
    }

    // TODO: Use std::optional or do not pass this as an argument
    void drawFrame(std::function<void()>& uiEvents) {
        vkWaitForFences(vkb_device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult currentResult = vkAcquireNextImageKHR(vkb_device, vkb_swapchain,UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame],
                                                VK_NULL_HANDLE, &imageIndex);
        if (currentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            ImGui_ImplVulkan_SetMinImageCount(chainMinImageCount);
            recreateSwapChain();
            return;
        } else if (currentResult != VK_SUCCESS && currentResult != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Draw UI
        drawImGui(uiEvents);

        updateCameraData();
        updateUniformBuffer(currentFrame);

        vkResetFences(vkb_device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordRenderCommandBuffer(commandBuffers[currentFrame], imageIndex);


        std::vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphores[currentFrame]};
        std::vector<VkPipelineStageFlags> stageMask = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::vector<VkCommandBuffer> commandBuffer = {commandBuffers[currentFrame]};
        std::vector<VkSemaphore> signalSemaphores = {renderFinishedSemaphores[currentFrame]};

        VkSubmitInfo submitInfo = vk::getSubmitInfo(waitSemaphores, stageMask, commandBuffer, signalSemaphores);

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        std::vector<VkSwapchainKHR> swapChains = {vkb_swapchain};

        VkPresentInfoKHR presentInfo = vk::getPresentInfoKHR(signalSemaphores, swapChains, imageIndex);
        currentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (currentResult == VK_ERROR_OUT_OF_DATE_KHR || currentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (currentResult != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // Work in progress! Render individual nodes with position offsets
    // as push constants.
    void renderNodes(const VkCommandBuffer commandBuffer, const ale::Model& model) {
        for (auto nodeId : model.rootNodes) {
            renderNode(commandBuffer, model.nodes[nodeId], model);
        }
    }

    void renderNode(const VkCommandBuffer commandBuffer, const ale::Node& node, const ale::Model& model) {
        // TODO: Add frustum culling

        if (node.mesh > -1 && node.bVisible == true) {
            // Get unique node id
            float objId = static_cast<float>(node.id);
            // Assign a unique "color" by object's id
            float perObjColorData[4] = {objId,1,1,1};
            auto t = node.transform;
            float* transform = ale::geo::glmMatToPtr(t);
            auto transRange = pushConstantRanges[0];
            auto colorRange = pushConstantRanges[1];

            vkCmdPushConstants(commandBuffer, pipelineLayout,transRange.stageFlags, transRange.offset,transRange.size,transform);
            vkCmdPushConstants(commandBuffer, pipelineLayout,colorRange.stageFlags, colorRange.offset, colorRange.size, perObjColorData);

            // Load node mesh
            MeshBufferData meshData = meshBuffers[node.mesh];
            vkCmdDrawIndexed(commandBuffer, meshData.size, 1, meshData.offset, 0, 0);
        }

        for(auto childNodeId : node.children) {
                assert(childNodeId > -1);
                renderNode(commandBuffer, model.nodes[childNodeId], model);
        }
    }

    void cleanup() {
        // Stop the device for cleanup
        vkDeviceWaitIdle(vkb_device);

        // CLEAN IMGUI
        // START
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        // END

        cleanupSwapChain();

        vkDestroyPipeline(vkb_device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(vkb_device, pipelineLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(vkb_device, uniformBuffers[i], nullptr);
            vkFreeMemory(vkb_device, uniformBuffersMemory[i], nullptr);
        }

        // TODO: Create helper functions for deallocations
        vkDestroyDescriptorPool(vkb_device, descriptorPool, nullptr);
        vkDestroyDescriptorPool(vkb_device, imguiPool, nullptr);

        vkDestroySampler(vkb_device, textureSampler, nullptr);
        vkDestroyImageView(vkb_device, textureImageView, nullptr);

        vkDestroyImage(vkb_device, textureImage, nullptr);
        vkFreeMemory(vkb_device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(vkb_device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(vkb_device, indexBuffer, nullptr);
        vkFreeMemory(vkb_device, indexBufferMemory, nullptr);

        vkDestroyBuffer(vkb_device, vertexBuffer, nullptr);
        vkFreeMemory(vkb_device, vertexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(vkb_device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(vkb_device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(vkb_device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(vkb_device, commandPool, nullptr);

        // TODO: Move here as many things as possible
        while (!destructorStack.empty()) {
            destructorStack.top()();
            destructorStack.pop();
        }

    }

    bool shouldClose() {
        return glfwWindowShouldClose(window);
    };

    GLFWwindow* getWindow() {
        return window;
    }

    UniformBufferObject getUbo(){
        return ubo;
    }

    void pushToUIDrawQueue(std::pair<std::vector<glm::vec3>, ale::UI_DRAW_TYPE>  pair) {
        uiDrawQueue.push_back(pair);
    }

    void flushUIDrawQueue() {
        uiDrawQueue.clear();
    }

    glm::vec2 getDisplaySize() {
        auto ds = ImGui::GetIO().DisplaySize;
        return {ds.x, ds.y};
    }













private:
    GLFWwindow* window;

    ale::sp<Camera> mainCamera = ale::to_sp<Camera>();

    UniformBufferObject ubo{};

    // GLFW variables

    vkb::Instance vkb_instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    vkb::PhysicalDevice vkb_physicalDevice;
    vkb::Device vkb_device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    vkb::Swapchain vkb_swapchain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;


    std::stack<std::function<bool()>> destructorStack = {};
    // TODO: add multiple model handling and scene hierarchy

    ale::Model& model;

    // TODO: Render materials instead of raw textures
    ale::Image image;

    glm::vec4 _posLight = glm::vec4(30.0, 40.0, 100.0, 1.0);

    // binding descriptions for ale::Vertex
    VkVertexInputBindingDescription ale_VertexBindingDescription = vk::getVertBindingDescription(0, sizeof(ale::Vertex));

    // attribute descriptions for ale::Vertex
    std::vector<VkVertexInputAttributeDescription> ale_VertexAttributeDescriptions = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(ale::Vertex, pos),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(ale::Vertex, color),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(ale::Vertex, texCoord),
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(ale::Vertex, normal),
        }
    };


    // TODO: This seems like a specific config. Might be useful to
    // move to a separate header
    std::vector<VkPushConstantRange> pushConstantRanges = {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = 64,
        },
        {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 64,
            .size = 16,
        },
    };

    struct Buffer {
        VkBuffer vulkanaBuffer;
        VkDeviceMemory memory;
        void* handle;
    };


    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    void* vertexBufferHandle;

    VkBuffer indexBuffer;
    uint32_t indexCount;
    VkDeviceMemory indexBufferMemory;
    void* indexBufferHandle;

    // An array of offsets for vertices of each mesh
    std::vector<MeshBufferData> meshBuffers;

    // Number of images in the swap chain
    int chainImageCount;
    int chainMinImageCount;

    // Some UI variables


    // An array of vertices to draw for ImGui
    std::vector<std::pair<std::vector<glm::vec3>, UI_DRAW_TYPE>> uiDrawQueue;

    // end UI

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorPool imguiPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                  VkDebugUtilsMessengerEXT debugMessenger,
                                  const VkAllocationCallbacks *pAllocator) {

        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    // FIXME: causes stack-use-after-return. investigate
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        // Renderer* app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        // app->framebufferResized = true;
    }


    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void cleanupSwapChain() {
        vkDestroyImageView(vkb_device, depthImageView, nullptr);
        vkDestroyImage(vkb_device, depthImage, nullptr);
        vkFreeMemory(vkb_device, depthImageMemory, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(vkb_device, imageView, nullptr);
        }

        vkb::destroy_swapchain(vkb_swapchain);
    }


    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(vkb_device);

        cleanupSwapChain();
        createSwapChain();
        createDepthResources();
    }

    void createInstance() {
        vkb::InstanceBuilder builder;

        builder.set_app_name("Antilegacy Editor")
                               .use_default_debug_messenger()
                               .set_engine_name("No engine")
                               .require_api_version(1,3);

        auto system_info_ret = vkb::SystemInfo::get_system_info();

        if (!system_info_ret) {
            throw std::runtime_error( system_info_ret.error().message());
        }


        // Enable validation layers if needed
        auto system_info = system_info_ret.value();
        if (renderer_enableValidationLayers) {
            if (system_info.validation_layers_available){
                builder.enable_validation_layers();
                trc::log("Enabling validation layers.");
            }else{
                throw std::runtime_error("validation layers requested, but not available!");
            }
        }
        // Build vulkan instance
        auto instance_res = builder.build();


        if (!instance_res) {
            throw std::runtime_error("failed to create instance!");
        }

        vkb_instance = instance_res.value();

        destructorStack.push([this](){
            vkb::destroy_instance(vkb_instance);
            return false;
        });

    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
        };
    }

    void setupDebugMessenger() {
        if (!renderer_enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(vkb_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }

        destructorStack.push([this](){
            if (renderer_enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(vkb_instance, debugMessenger, nullptr);
            }
            return false;
        });
    }

    void createSurface() {
        if (glfwCreateWindowSurface(vkb_instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        destructorStack.push([this](){
            vkb::destroy_surface(vkb_instance, surface);
            return false;
        });
    }

    /*
    Picks a physical device according to its rating for the physicalDevice field.
    TODO: Add overrides and handlers for manual device selection
    */
    void pickPhysicalDevice() {

        unsigned int deviceCount = 0;
        vkEnumeratePhysicalDevices(vkb_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        VkPhysicalDeviceVulkan13Features features{
            .dynamicRendering = true,
            /* .synchronization2 = true, */
        };




        vkb::PhysicalDeviceSelector phys_device_selector(vkb_instance);
        auto physical_device_selector_return = phys_device_selector
                .add_required_extensions(deviceExtensions.size(), deviceExtensions.data())
                .set_required_features_13(features)
                .set_surface(surface)
                .select();

        if (!physical_device_selector_return) {
            throw std::runtime_error("failed to pick a device!");
        }

        vkb_physicalDevice = physical_device_selector_return.value();


    }

    void createLogicalDevice() {
        vkb::DeviceBuilder builder{vkb_physicalDevice};

        VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT ext {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT,
            .dynamicRenderingUnusedAttachments = true,
        };

        auto dev_ret = builder
            .add_pNext(&ext)
            .build();

        if (!dev_ret) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkb_device = dev_ret.value();

        auto queue_graphics = vkb_device.get_queue(vkb::QueueType::graphics);
        if (!queue_graphics) {
            throw std::runtime_error("failed to find graphics queue!");
        }

        auto queue_present = vkb_device.get_queue(vkb::QueueType::present);
        if (!queue_present) {
            throw std::runtime_error("failed to find present queue!");
        }

        graphicsQueue = queue_graphics.value();
        presentQueue = queue_present.value();

        destructorStack.push([this](){
            vkb::destroy_device(vkb_device);
            return false;
        });
    }

    void createSwapChain() {
        vkb::SwapchainBuilder swapchain_builder{ vkb_device };
        auto swap_ret = swapchain_builder
                        .build();

        if (!swap_ret){
            throw std::runtime_error("failed to create swap chain!");
        }

        vkb_swapchain = swap_ret.value();

        // Variables for ImGUI
        chainImageCount = vkb_swapchain.image_count;
        chainMinImageCount = vkb_swapchain.image_count;

        swapChainImages = vkb_swapchain.get_images().value();
        swapChainImageViews = vkb_swapchain.get_image_views().value();

        swapChainImageFormat =  vkb_swapchain.image_format;
        swapChainExtent = vkb_swapchain.extent;

        // destructorStack.push([this](){
        //     vkb::destroy_swapchain(vkb_swapchain);
        //     return false;
        // });
    }



    void createDescriptorSetLayout() {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};

        vk::pushBackDescriptorSetBinding(layoutBindings, 1,
                                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_VERTEX_BIT);
        vk::pushBackDescriptorSetBinding(layoutBindings, 1,
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         VK_SHADER_STAGE_FRAGMENT_BIT);

        auto layoutInfo = vk::getDescriptorSetLayout(layoutBindings);

        if (vkCreateDescriptorSetLayout(vkb_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = Loader::getFileContent("shaders/vert.spv");
        auto fragShaderCode = Loader::getFileContent("shaders/frag.spv");

        auto vertShaderModule = vk::createShaderModule(vkb_device, vertShaderCode);
        auto fragShaderModule = vk::createShaderModule(vkb_device, fragShaderCode);

        std::string sMainStage = "main";

        auto vertShaderStageInfo = vk::getShaderStageInfo(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, sMainStage);
        auto fragShaderStageInfo = vk::getShaderStageInfo(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, sMainStage);

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto vertexInputInfo = vk::getVertexInputInfo(ale_VertexBindingDescription,
                                                    ale_VertexAttributeDescriptions);
        auto inputAssembly = vk::getDefaultInputAssembly();
        auto viewportState = vk::getDefaultViewportState();
        auto rasterizer = vk::getDefaultRasterizer();
        auto multisampling = vk::getDefaultMultisampling();
        auto depthStencil = vk::getDefaultDepthStencil();

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = 	VK_COLOR_COMPONENT_R_BIT |
								VK_COLOR_COMPONENT_G_BIT |
								VK_COLOR_COMPONENT_B_BIT |
								VK_COLOR_COMPONENT_A_BIT,
        };

        auto colorBlending = vk::getColorBlending(colorBlendAttachment);

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        auto dynamicState = vk::getPipelineDynamicState(dynamicStates);
        auto pipelineLayoutInfo = vk::getPipelineLayout(descriptorSetLayout, pushConstantRanges);

        if (vkCreatePipelineLayout(vkb_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

        VkPipelineRenderingCreateInfo renderingCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &vkb_swapchain.image_format,
            .depthAttachmentFormat = findDepthFormat(),

        };

        VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingCreateInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
        };


        if (vkCreateGraphicsPipelines(vkb_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(vkb_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkb_device, vertShaderModule, nullptr);
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vkb_physicalDevice);

        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
        };

        if (vkCreateCommandPool(vkb_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(vkb_physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createTextureImage() {

        VkDeviceSize imageSize = image.w * image.h * 4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // TODO: store this pointer for later use
        void* data;
        vkMapMemory(vkb_device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
            memcpy(data, &image.data.at(0), static_cast<size_t>(imageSize));
        vkUnmapMemory(vkb_device, stagingBufferMemory);

        createImage(image.w, image.h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(image.w), static_cast<uint32_t>(image.h));
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(vkb_device, stagingBuffer, nullptr);
        vkFreeMemory(vkb_device, stagingBufferMemory, nullptr);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureSampler() {
        VkPhysicalDeviceProperties2 props{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(vkb_physicalDevice, &props);

        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER VK_SAMPLER_ADDRESS_MODE_REPEAT
        auto sampler_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = sampler_mode,
            .addressModeV = sampler_mode,
            .addressModeW = sampler_mode,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = props.properties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        if (vkCreateSampler(vkb_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        VkImageView imageView;
        if (vkCreateImageView(vkb_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo = vk::getImageInfo(width, height, format, tiling, usage);

        if (vkCreateImage(vkb_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(vkb_device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vkb_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(vkb_device, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;


        // FIXME: Too hardcoded, pass this as parameters
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {
                width,
                height,
                1
            },
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer() {

        if (model.meshes.size() <= 0) {
            throw std::runtime_error("No meshes in the model!");
        }

        // Counter for all the vertices in the model
        VkDeviceSize numAllVerts = 0;
        // Check vertex size against this vert
        auto vert = model.meshes[0].vertices[0];

        // Add mesh sizes to the counter
        for(const auto& m: model.meshes) {
            numAllVerts += m.vertices.size();
        }

        if (numAllVerts <= 0) {
            throw std::runtime_error("Vertex buffer size is 0!");
        }

        VkDeviceSize vertBufferSize = numAllVerts * sizeof(vert);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(vertBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     stagingBuffer, stagingBufferMemory);


        // Create a huge array with all the vertices
        std::vector<ale::Vertex> allVertices = {};
        allVertices.reserve(numAllVerts);

        // TODO: Use this data to map vertices for editing

        // Populate our giant vertex array
        for (auto m : model.meshes) {
            for (auto v: m.vertices) {
                allVertices.push_back(v);
            }
        }

        vkMapMemory(vkb_device, stagingBufferMemory, 0, vertBufferSize, 0, &vertexBufferHandle);
        /// MEMORY MAPPED

        // Map our gigantic vertex array straight to gpu memory
            memcpy(vertexBufferHandle, allVertices.data(), vertBufferSize);

        vkUnmapMemory(vkb_device, stagingBufferMemory);
        /// MEMORY UNMAPPED

        createBuffer(vertBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     vertexBuffer, vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, vertBufferSize);

        vkDestroyBuffer(vkb_device, stagingBuffer, nullptr);
        vkFreeMemory(vkb_device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {

        unsigned long numIdx = 0;
        indexCount = 0;

        // Samples the first index for default size
        auto index = model.meshes[0].indices[0];

        if (model.meshes.size() <= 0) {
            throw std::runtime_error("No meshes in the model!");
        }

        for(const auto& m: model.meshes) {
            numIdx += m.indices.size();
            indexCount += m.indices.size();
        }

        if (numIdx<= 0) {
            throw std::runtime_error("Vertex buffer size is 0!");
        }

        std::vector<unsigned int> allIdx = {};
        allIdx.reserve(numIdx * sizeof(index));

        unsigned int iOffset = 0;
        unsigned int vOffset = 0;
        for (auto& m : model.meshes) {
            unsigned int mSize = m.indices.size();
            unsigned int vSize = m.vertices.size();

            for (auto i : m.indices) {
                allIdx.push_back(i + vOffset);
            }

            MeshBufferData data {
                .size = mSize,
                .offset = iOffset,
            };

            meshBuffers.push_back(data);
            iOffset += mSize;
            vOffset += vSize;
        }

        VkDeviceSize bufferSize = numIdx * sizeof(index);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 stagingBuffer, stagingBufferMemory);

        vkMapMemory(vkb_device, stagingBufferMemory, 0, bufferSize, 0, &indexBufferHandle);

            memcpy(indexBufferHandle, allIdx.data(), bufferSize);

        vkUnmapMemory(vkb_device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(vkb_device, stagingBuffer, nullptr);
        vkFreeMemory(vkb_device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(vkb_device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        // First UBO is for MVP + Light position
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // Texture sampler
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };

        if (vkCreateDescriptorPool(vkb_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .pSetLayouts = layouts.data(),
        };

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vkb_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo mvpUboInfo {
                .buffer = uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject),
            };

            VkDescriptorImageInfo textureImageInfo{
                .sampler = textureSampler,
                .imageView = textureImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };


            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &mvpUboInfo,
            };

            descriptorWrites[1] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &textureImageInfo,
            };

            vkUpdateDescriptorSets(vkb_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        if (vkCreateBuffer(vkb_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkb_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
        };

        if (vkAllocateMemory(vkb_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(vkb_device, buffer, bufferMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vkb_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
        };

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(vkb_device, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{ .size = size };

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vkb_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = (uint32_t) commandBuffers.size(),
        };

        if (vkAllocateCommandBuffers(vkb_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordRenderCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRect2D renderAreaWholeViewport = { .offset = {0, 0}, .extent = swapChainExtent, };

        VkImageSubresourceRange colorRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        vk::VulkanImageState topOfPipeState {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .stage =  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        };

        vk::VulkanImageState colorAttachmentState {
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .stage =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        vk::VulkanImageState finalPresentState {
            .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ,
            .stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        };

        vk::addPipelineBarrier(commandBuffer, swapChainImages[imageIndex],
                {.dst = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
                colorRange, topOfPipeState, colorAttachmentState);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};


        VkRenderingAttachmentInfo sceneColorAttachmentInfo =
            vk::getRenderingAttachment(swapChainImageViews[imageIndex],
                                       VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                                       VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       VK_ATTACHMENT_STORE_OP_STORE,
                                       clearValues[0]);


        VkRenderingAttachmentInfo sceneDepthAttachmentInfo =
            vk::getRenderingAttachment(depthImageView,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                       clearValues[1]);

        VkRenderingInfo renderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = VK_NULL_HANDLE,
            .renderArea = renderAreaWholeViewport,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &sceneColorAttachmentInfo,
            .pDepthAttachment = &sceneDepthAttachmentInfo,
        };

        // **Render 3D scene**
        vkCmdBeginRendering(commandBuffer, &renderingInfo);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport viewport = vk::getViewport(swapChainExtent);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{.offset = {0, 0}, .extent = swapChainExtent};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};

            // Bind all vertices to one buffer
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1,
                                    &descriptorSets[currentFrame], 0, nullptr);
            renderNodes(commandBuffer, model);

        vkCmdEndRendering(commandBuffer);


        VkRenderingAttachmentInfo imguiColorAttachmentInfo =
            vk::getRenderingAttachment(swapChainImageViews[imageIndex],
                                        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                                        VK_ATTACHMENT_LOAD_OP_LOAD,
                                        VK_ATTACHMENT_STORE_OP_STORE,
                                        clearValues[0]);

        VkRenderingInfo imguiRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = VK_NULL_HANDLE,
            .renderArea = renderAreaWholeViewport,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &imguiColorAttachmentInfo,
        };

        // Render ImGui interface to the scene attachment
        vkCmdBeginRendering(commandBuffer, &imguiRenderingInfo);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);
        vkCmdEndRendering(commandBuffer);


        vk::addPipelineBarrier(commandBuffer, swapChainImages[imageIndex],
                {.src= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
                colorRange, colorAttachmentState, finalPresentState);


        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(vkb_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vkb_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(vkb_device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    // INIT IMGUI
    // START

    void setStyleImGui() {
        //TODO: move this code to a config file
        ImGuiStyle &style = ImGui::GetStyle();

        // Convert four ints to ImVec4 color
        auto _clr = [](int r, int g, int b, int a) {
            float RANGE = 255;
            return ImVec4( r/RANGE,g/RANGE,b/RANGE,a/RANGE);
        };


        // Change colors of elements to make ImGui bearable
        style.Colors[ImGuiCol_::ImGuiCol_FrameBg] = _clr(4,4,4,255);
        style.Colors[ImGuiCol_::ImGuiCol_WindowBg] = _clr(14,22,22,255);
        style.Colors[ImGuiCol_::ImGuiCol_Tab] = _clr(14,22,22,255);
        style.Colors[ImGuiCol_::ImGuiCol_Button] = _clr(38,48,48,255);
        style.Colors[ImGuiCol_::ImGuiCol_ButtonActive] = _clr(30,78,78,255);
        style.Colors[ImGuiCol_::ImGuiCol_SliderGrab] = _clr(20,68,68,255);
        style.Colors[ImGuiCol_::ImGuiCol_ButtonHovered] =_clr(108,108,108,255);


        style.WindowMinSize        = ImVec2( 100, 20 );
        style.ItemSpacing          = ImVec2( 6, 2 );
        style.Alpha                = 0.95f;
        style.WindowRounding       = 2.0f;
        style.FrameRounding        = 2.0f;
        style.IndentSpacing        = 6.0f;
        style.ItemInnerSpacing     = ImVec2( 1, 1 );
        style.ColumnsMinSpacing    = 50.0f;
        style.GrabMinSize          = 14.0f;
        style.GrabRounding         = 16.0f;
        style.ScrollbarSize        = 12.0f;
        style.ScrollbarRounding    = 16.0f;
        style.ScaleAllSizes(4);

        float size_pixels = 22;
        ImFontConfig config;
        ImGuiIO _io = ImGui::GetIO();
        _io.Fonts->Clear();
        _io.Fonts->AddFontFromFileTTF("./fonts/gidole-regular.ttf", size_pixels, &config );
    }

    void initImGUI(){
        trc::log("There is a minor memory leak here!",trc::WARNING);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& _io = ImGui::GetIO();
        _io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard
        _io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable docking
        _io.ConfigDockingWithShift = true; // Hold shift to dock

        // Set up style
        ImGui::StyleColorsDark();
        setStyleImGui();

        VkDescriptorPoolSize pool_sizes[] {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        };

        VkDescriptorPoolCreateInfo pool_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1,
            .poolSizeCount = std::size(pool_sizes),
            .pPoolSizes = pool_sizes,
        };

        vkCreateDescriptorPool(vkb_device, &pool_info, nullptr, &imguiPool);

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info {
            .Instance = vkb_instance,
            .PhysicalDevice = vkb_physicalDevice,
            .Device = vkb_device,
            .QueueFamily = 0,
            .Queue = graphicsQueue,
            .PipelineCache = VK_NULL_HANDLE,
            .DescriptorPool = imguiPool,
            .Subpass = 0,
            .MinImageCount = (uint32_t)(chainMinImageCount),
            .ImageCount = (uint32_t)(chainMinImageCount),
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .ColorAttachmentFormat = vkb_swapchain.image_format,
            .Allocator = VK_NULL_HANDLE,
            .CheckVkResultFn = nullptr,
        };

        ImGui_ImplVulkan_Init(&init_info, nullptr);
    }


    // Initializes ImGui stuff and records ui events here
    void drawImGui(std::function<void()>& uiEventsCallback) {
        /// INIT IMPORTANT IMGUI STUFF
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImGuiIO& _io = ImGui::GetIO();
        float x = 0, y = 0, w = _io.DisplaySize.x, h = _io.DisplaySize.y;
        ImGuizmo::SetRect(x, y, w, h);

        using ui = ale::UIManager;
        // Draw each selected face as a triangle

        MVP pvm  = ui::getMVPWithFlippedProjection({
            .m = ubo.model,
            .v = ubo.view,
            // Imgui works with -Y as up, ALE works with Y as up
            .p = ubo.proj,});

        for(auto pair: uiDrawQueue) {
            std::vector<glm::vec3> vec = pair.first;
            ale::UI_DRAW_TYPE type = pair.second;
            ui::drawVectorOfPrimitives(vec, type, pvm);
        }

        ui::drawDefaultWindowUI(mainCamera, this->model, pvm);

        uiEventsCallback();

        /// FINAL IMPORTANT STUFF
        ImGui::Render();
    }

    void updateCameraData() {
        // NOTE: for now the "up" axis is Y
        const glm::vec3 GLOBAL_UP = glm::vec3(0,1,0);
        auto yawPitch = mainCamera->getYawPitch();
        auto camPos = mainCamera->getPos();

        // TODO: get this data from the node transform matrix
        // Model matrix
        ubo.model =  glm::rotate(glm::mat4(1.0f),
                                glm::radians(1.5f),
                                glm::vec3(1.0f, 0.0f, 0.0f));

        // View matrix
        if (mainCamera->mode == CameraMode::FREE) {
            glm::mat4x4 view(1.0f);

            float yawAng = glm::radians(yawPitch.x);

            // Generate pitch vector based on the yaw angle
            glm::vec3 pitchVec = glm::vec3(glm::cos(yawAng), 0.0f, glm::sin(yawAng));
            float pitchAng = glm::radians(yawPitch.y);

            view = glm::rotate(view, yawAng, GLOBAL_UP);
            view = glm::rotate(view, pitchAng, pitchVec);
            view = glm::translate(view, -camPos);

            ubo.view = view;
        } else {
            ubo.view = glm::lookAt(camPos, mainCamera->getTarget(), GLOBAL_UP);
        }

        // Porjection matrix
        ubo.proj = glm::perspective(glm::radians(mainCamera->getFov()),
                        swapChainExtent.width / (float) swapChainExtent.height,
                        mainCamera->getPlaneNear(), mainCamera->getPlaneFar());

        ubo.proj[1][1] *= -1;

        ubo.light = _posLight;
    }


    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }


    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        std::optional<unsigned int> computeFamily;

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        if (!computeFamily.has_value()) {
            trc::log("No support for compute shaders detected!", trc::WARNING);
        }

        return indices;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};
} // namespace ale
#endif //ALE_RENDERER
