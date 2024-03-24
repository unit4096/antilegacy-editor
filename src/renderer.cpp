#include <renderer.h>


Renderer::Renderer(ale::Model& _model):model(_model){
    // FIXME: For now, treats the first texture as the default tex for all meshes.
    image = model.textures[0];
};


void Renderer::initWindow() {
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

void Renderer::initRenderer() {
    initVulkan();
    initImGUI();
}


void Renderer::bindCamera(ale::sp<Camera> cam) {
    this->mainCamera = cam;
}

sp<ale::Camera> Renderer::getCurrentCamera() {
    return mainCamera;
}

// TODO: Use std::optional or do not pass this as an argument
void Renderer::drawFrame(std::function<void()>& uiEvents) {

    vkWaitForFences(vkb_device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vkb_device, vkb_swapchain,UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        ImGui_ImplVulkan_SetMinImageCount(chainMinImageCount);
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Draw UI
    drawImGui(uiEvents);

    updateCameraData();
    updateUniformBuffer(currentFrame);

    vkResetFences(vkb_device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);


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
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Work in progress! Render individual nodes with position offsets
// as push constants.
void Renderer::renderNodes(const VkCommandBuffer commandBuffer, const ale::Model& model) {
    for (auto nodeId : model.rootNodes) {
        renderNode(commandBuffer, model.nodes[nodeId], model);
    }
}

void Renderer::renderNode(const VkCommandBuffer commandBuffer, const ale::Node& node, const ale::Model& model) {
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

void Renderer::cleanup() {
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

bool Renderer::shouldClose() {
    return glfwWindowShouldClose(window);
};

GLFWwindow* Renderer::getWindow() {
    return window;
}

UniformBufferObject Renderer::getUbo(){
    return ubo;
}

void Renderer::pushToUIDrawQueue(std::pair<std::vector<glm::vec3>, ale::UI_DRAW_TYPE>  pair) {
    uiDrawQueue.push_back(pair);
}

void Renderer::flushUIDrawQueue() {
    uiDrawQueue.clear();
}

glm::vec2 Renderer::getDisplaySize() {
    auto ds = ImGui::GetIO().DisplaySize;
    return {ds.x, ds.y};
}

