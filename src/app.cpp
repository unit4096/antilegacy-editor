#include "app.h"


App::App(AppConfigData config) {
    this->_config = config;
}


/*
    Runs the editor. It connects every other class
    whether it is window creation or input management.
    As of [14.01.2024] it is intentionally verboose
    for the ease of development
*/
int App::run() {

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

        // Loader handles system IO
        ale::Loader loader;

        loader.recordCommandLineArguments(_config.argc, _config.argv);
        loader.getFlaggedArgument("-f", model_path);

        /*
        Model object that represents a scene with multiple nodes,
        meshes, and textures
        */
        ale::Model model;

        // // You can use this to load a custom texture
        // loader.loadTexture("RELATIVE_PATH_TO_TEX", image);

        // Load a GLTF model
        if (loader.loadModelGLTF(model_path, model)) {
            trc::log("Cannot load model by path: " + model_path, trc::ERROR);
            return 1;
        }

        // Create Vulkan renderer object
        ale::Renderer renderer(model);

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


        // Dynamically set camera position by mesh bounding box

        assert(model.meshes.size() > 0);

        // Uses AABB of the first mesh
        if (model.meshes[0].minPos.size() >= 3) {
            assert(model.meshes[0].minPos.size() ==
                   model.meshes[0].maxPos.size());

            float sizeZ = std::abs(model.meshes[0].minPos[2] - model.meshes[0].maxPos[2]);

            float frontEdge = model.meshes[0].minPos[2] - sizeZ;
            float middleY = (model.meshes[0].minPos[1] +
                             model.meshes[0].maxPos[1]) / 2;

            mainCam->setPosition(glm::vec3(0, middleY, -frontEdge));
        } else {
            mainCam->setPosition(glm::vec3(0, 50, 150));
        }

        mainCam->setSpeed(1.0f);

        // Further code tests camera movement
        // TODO: move this code from the main file, provide better handling for bindings


        std::chrono::duration<double> deltaTime;

        float cameraSpeedAdjusted = 1.0f;

        float mouseSensitivity = 0.06f;

		// Movement along global XYZ aixs
        // auto moveX  = [&]() {mainCam->movePosGlobal( glm::vec3(0,0,1), cameraSpeed);};
        // auto moveNX = [&]() {mainCam->movePosGlobal(glm::vec3(0,0,-1), cameraSpeed);};
        auto moveY  = [&]() {mainCam->movePosGlobal( glm::vec3(0,1,0), cameraSpeedAdjusted);};
        auto moveNY = [&]() {mainCam->movePosGlobal(glm::vec3(0,-1,0), cameraSpeedAdjusted);};
        // auto moveZ  = [&]() {mainCam->movePosGlobal( glm::vec3(1,0,0), cameraSpeed);};
        // auto moveNZ = [&]() {mainCam->movePosGlobal(glm::vec3(-1,0,0), cameraSpeed);};

		// WASD free camera movement
        auto moveF = [&]() { mainCam->moveForwardLocal(cameraSpeedAdjusted);};
        auto moveB = [&]() {mainCam->moveBackwardLocal(cameraSpeedAdjusted);};
        auto moveL = [&]() {    mainCam->moveLeftLocal(cameraSpeedAdjusted);};
        auto moveR = [&]() {   mainCam->moveRightLocal(cameraSpeedAdjusted);};


        ale::geo::REMesh reMesh;
        loader.populateREMesh(model.meshes[0], reMesh);

        // This function tests raycasts from the middle of the screen
        auto raycast = [&]() {
            bool result = false;
            glm::vec3 forward = mainCam->getForwardOrientation();

            forward *= -1;

            glm::vec3 out_intersection_point = glm::vec3(0);

            for (auto f : reMesh.faces) {
                result = geo::rayIntersectsTriangle(
                                    mainCam->getPos(),
                                    forward,
                                    f, out_intersection_point);
                if (result) {
                    break;
                }
            }

            ale::Tracer::raw << "Raycast result: " << result << "\n";
        };

        // Bind lambda functions to keyboard actions
        input.bindFunction(ale::InputAction::CAMERA_MOVE_F, moveF, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_B, moveB, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_L, moveL, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_R, moveR, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_U, moveY, true);
        input.bindFunction(ale::InputAction::CAMERA_MOVE_D,moveNY, true);
        input.bindFunction(ale::InputAction::FUNC_1,raycast, false);
        // Bind global camera to the inner camera object
        renderer.bindCamera(mainCam);
        renderer.initRenderer();


        // Main rendering loop. Queries the input and renders to a window
        while (!renderer.shouldClose()) {
            // Time point to the frame start
            auto thisFrame = std::chrono::steady_clock::now();
            // Delta time for editor calculations
            deltaTime = duration_cast<std::chrono::duration<double>>(thisFrame - lastFrame);
            lastFrame = thisFrame;

            cameraSpeedAdjusted = mainCam->getSpeed() * deltaTime.count() * 100 / 8;

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
            const std::chrono::duration<double, std::milli> remainder = fps_cap - frameDuration;
            // NOTE: on some hardware enabled VSync will slow down the
            // system to sleep_for + VSync wait. In this case just
            // disable this fps cap
            std::this_thread::sleep_for(remainder);
        }
        renderer.cleanup();

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
