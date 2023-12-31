// ext
#include <stdexcept>
#include <memory>
#include <chrono>
#include <thread>

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


// int
#include <input_manager.h>
#include <camera.h>
#include <renderer.h>
#include <tracer.h>
#include <model_loader.h>



int main(int argc, char **argv) {

    try {

        // Time management for framerate control
        // FPS cap in 8 milliseconds -> 120 FPS
        const std::chrono::duration<double, std::milli> fps_cap(8);
        auto lastFrame = std::chrono::steady_clock::now();


        // Set up logging
        std::vector<trc::LogLevel> logLevels = {
            trc::LogLevel::DEBUG,
            trc::LogLevel::INFO,
            trc::LogLevel::WARNING,
            trc::LogLevel::ERROR,
        };    
        trc::SetLogLevels(logLevels);

        std::string model_path = "./models/fox/Fox.gltf";
        ale::Loader loader;
        
        loader.recordCommandLineArguments(argc, argv);
        
        if (loader.cmdOptionExists("-f")) {
            std::string file = loader.getCmdOption("-f");   
            trc::log("Loaded argument: " + file);         
            if (loader.isFileValid(file)) {
                model_path = file;
                trc::log("File can be loaded: " + file);
            } else {
                trc::log("File path is not valid, falling back to defaults!", trc::WARNING);
            }
        }

        
        

        // Main mesh object
        ale::Mesh mesh;
        // Main texture object
        ale::Image image;

        // Model paths
        // std::string model_cube_path = "./models/cube/Cube.gltf";
        // std::string model_fox_path = "./models/fox/Fox.gltf";

        // This .obj model should always load
        // std::string dummy_model_path = "models/viking_room.obj";
        // std::string dummy_texture_path = "textures/viking_room.png";
        // UV checker texture
        // const std::string uv_checker_path = "textures/tex_uv_checker.png";
        
        
        // Load GLTF models
        if (loader.loadModelGLTF(model_path, mesh, image)) {
            trc::log("Cannot load model by path: " + model_path, trc::ERROR);
            return 1;
        }

        // // Load default .obj model (should always work)
        // loader.loadModelOBJ(dummy_model_path.data(), mesh);
        // // Load default texture (should always work)
        // loader.loadTexture(dummy_texture_path.data(), image);
        
        // Load this texture to check UV layout        
        // loader.loadTexture(uv_checker_path.data(), image);


        // Create Vulkan renderer object
        ale::Renderer renderer(mesh,image);

        // Create input manager object
        ale::InputManager input;

        // Create a camera object that will be passed to the renderer
        std::shared_ptr<ale::Camera> mainCam = std::make_shared<ale::Camera>();

        // TODO: possibly move window management outside the renderer
        renderer.initWindow();
        input.init(renderer.getWindow());
        
        // Make the default mode to be FREE
        mainCam->toggleMode();
        // Set initial camera position
        mainCam->setPosition(glm::vec3(0, 50, 150));

        // Further code tests camera movement
        // TODO: move this code from the main file, provide better handling for bindings
        float cameraSpeed = 0.1f;
        float mouseSensitivity = 0.06f;

		// Movement along global XYZ aixs
        // auto moveX  = [&]() {mainCam->movePosGlobal( glm::vec3(0,0,1), cameraSpeed);};
        // auto moveNX = [&]() {mainCam->movePosGlobal(glm::vec3(0,0,-1), cameraSpeed);};
        auto moveY  = [&]() {mainCam->movePosGlobal( glm::vec3(0,1,0), cameraSpeed);};
        auto moveNY = [&]() {mainCam->movePosGlobal(glm::vec3(0,-1,0), cameraSpeed);};
        // auto moveZ  = [&]() {mainCam->movePosGlobal( glm::vec3(1,0,0), cameraSpeed);};
        // auto moveNZ = [&]() {mainCam->movePosGlobal(glm::vec3(-1,0,0), cameraSpeed);};

		// WASD free camera movement
        auto moveF = [&]() { mainCam->moveForwardLocal(cameraSpeed);};
        auto moveB = [&]() {mainCam->moveBackwardLocal(cameraSpeed);};
        auto moveL = [&]() {    mainCam->moveLeftLocal(cameraSpeed);};
        auto moveR = [&]() {   mainCam->moveRightLocal(cameraSpeed);};

        // Bind lambda functions to keyboard actions
        input.bindFunction(ale::InputAction::CAMERA_MOVE_F, moveF, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_B, moveB, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_L, moveL, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_R, moveR, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_U, moveY, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_D,moveNY, true);
        
        // Bind global camera to the inner camera object 
        renderer.bindCamera(mainCam);
        renderer.initRenderer();
        
        while (!renderer.shouldClose()) {
            // Time point to the frame start
            auto thisFrame = std::chrono::steady_clock::now();
            // Delta time for editor calculations
            auto deltaTime = duration_cast<std::chrono::duration<double>>(thisFrame - lastFrame);
            lastFrame = thisFrame;

            // polling events, callbacks fired            
            glfwPollEvents();
            
            // Key camera input
            input.executeActiveKeyActions();

            // Mouse camera input
            input.executeActiveMouseAcitons();            
            ale::v2d mouseMovement = input.getLastDeltaMouseOffset();            
            ale::v2f camYawPitch = mainCam->getYawPitch();
            camYawPitch.x-= mouseMovement.x * mouseSensitivity;
            camYawPitch.y-= mouseMovement.y * mouseSensitivity;
            mainCam->setYawPitch(camYawPitch.x,camYawPitch.y);

            // Drawing the results of the input   
            renderer.drawFrame();
            
            // Smooth framerate
            auto thisFrameEnd = std::chrono::steady_clock::now();
            auto frameDuration = duration_cast<std::chrono::duration<double>>(thisFrameEnd - thisFrame);

            // Sleep for = cap time - frame duration (to avoid FPS spikes)
            const std::chrono::duration<double, std::milli> elapsed = fps_cap - frameDuration;
            std::this_thread::sleep_for(elapsed);
        }
        renderer.cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}