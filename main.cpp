#include <iostream>
#include <stdexcept>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif //GLFW

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "model_loader/model_loader.h"

import renderer;


int main() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ale::Loader loader;
    Model model;
    Image image;


    std::string model_cube_path = "./models/cube/Cube.gltf";

    std::string dummy_model_path = "models/viking_room.obj";
    std::string dummy_texture_path = "textures/viking_room.png";
    const std::string uv_checker_path = "textures/tex_uv_checker.jpg";
    
    
    // Load GLTF models
    // loader.loadModelGLTF(model_cube_path, model, image);
    

    // Load dummy model (should always work)
    loader.loadModelOBJ(dummy_model_path.data(), model);
    // Load dummy texture (should always work)
    loader.loadTexture(dummy_texture_path.data(), image);
    
    // Load this texture to check UV layout
    // loader.loadTexture(uv_checker_path.data(), image);

    ale::Renderer renderer(io,model,image);

    try {
        renderer.initWindow();
        renderer.initCamera();
        renderer.initRenderer();
        
        while (!renderer.shouldClose()) {
            glfwPollEvents();    
            renderer.drawFrame();
        }
        renderer.cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}