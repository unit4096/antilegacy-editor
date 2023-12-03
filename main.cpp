#include <iostream>
#include <stdexcept>
#include <memory>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif //GLFW

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <input_manager.h>
#include <camera.h>
#include <renderer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "model_loader/model_loader.h"



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
    ale::InputManager input;
    // A camera object that will be passed to the renderer
    std::shared_ptr<ale::Camera> currentCamera = std::make_shared<ale::Camera>();

    try {
        // TODO: possibly move window management outside the renderer
        renderer.initWindow();
        input.init(renderer.getWindow());

        // Further code tests movement along 3 global axis
        // TODO: move this code from the main file, provide better handling for bindings
        
        int dummy_camera_speed = 1.0f;

        auto moveX = [&]() {
            currentCamera->movePosGlobal(glm::vec3(1,0,0), dummy_camera_speed);
        };
        auto moveNX = [&]() {
            currentCamera->movePosGlobal(glm::vec3(-1,0,0), dummy_camera_speed);
        };
        auto moveY = [&]() {
            currentCamera->movePosGlobal(glm::vec3(0,1,0), dummy_camera_speed);
        };
        auto moveNY = [&]() {
            currentCamera->movePosGlobal(glm::vec3(0,-1,0), dummy_camera_speed);
        };
        auto moveZ = [&]() {
            currentCamera->movePosGlobal(glm::vec3(0,0,1), dummy_camera_speed);
        };
        auto moveNZ = [&]() {
            currentCamera->movePosGlobal(glm::vec3(0,0,-1), dummy_camera_speed);
        };
        input.bindFunction(ale::InputAction::CAMERA_MOVE_F,moveX);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_B,moveNX);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_L,moveZ);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_R,moveNZ);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_U,moveY);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_D,moveNY);
        
        // Bind global camera to the inner camera object 
        renderer.bindCamera(currentCamera);
        renderer.initRenderer();
        
        while (!renderer.shouldClose()) {
            // polling events, callbacks fired
            glfwPollEvents();
            // Drawing the results of the input   
            renderer.drawFrame();
        }
        renderer.cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}