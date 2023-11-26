#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#ifndef ALE_LOADER
#define ALE_LOADER
#include <model_loader.h>
#endif //ALE_LOADER

import renderer;


int main() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ale::Loader loader;
    Model model;
    Image image;


    // std::string model_cube_path = "./models/cube/Cube.gltf";

    std::string dummy_model_path = "models/viking_room.obj";
    std::string dummy_texture_path = "textures/viking_room.png";
    
    
    
    // loader.loadModelGLTF(model_cube_path,model.indices, model.vertices);
    
    loader.loadModelOBJ(dummy_model_path.data(), model.indices, model.vertices);
    
    loader.loadTexture(dummy_texture_path.data(), image);

    ale::Renderer renderer(io,model,image);

    try {
        renderer.init();
        
        // FIXME: closing using windowShouldClose generates vk errors in cleanup
        while (!renderer.shouldClose()) {
            glfwPollEvents();    
            renderer.drawFrame();
        } 
        renderer.drawFrame();
        renderer.cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}