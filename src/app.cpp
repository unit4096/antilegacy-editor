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
        // FRAMERATE MANAGEMENT
        // FPS cap in 8 milliseconds -> 120 FPS
        const std::chrono::duration<double, std::milli> fps_cap(8);
        auto lastFrame = std::chrono::steady_clock::now();
        std::chrono::duration<double> deltaTime;


        // LOGGING
        std::vector<trc::LogLevel> logLevels = {
            trc::LogLevel::DEBUG,
            trc::LogLevel::INFO,
            trc::LogLevel::WARNING,
            trc::LogLevel::ERROR,
        };
        trc::SetLogLevels(logLevels);


        // MODEL LOADING
        std::string model_path = "./models/fox/Fox.gltf";
        ale::Loader loader;
        loader.recordCommandLineArguments(_config.argc, _config.argv);
        loader.getFlaggedArgument("-f", model_path);

        ale::Model model;
        // // You can use this to load a custom texture
        // loader.loadTexture("RELATIVE_PATH_TO_TEX", image);

        // Load a GLTF model
        if (loader.loadModelGLTF(model_path, model)) {
            trc::log("Cannot load model by path: " + model_path, trc::ERROR);
            return 1;
        }


        // RENDERING
        // Create Vulkan renderer object
        ale::Renderer renderer(model);
        renderer.initWindow();

        // Create a camera object that will be passed to the renderer
        std::shared_ptr<ale::Camera> mainCam = std::make_shared<ale::Camera>();
        // Make the default mode to be FREE
        mainCam->toggleMode();


        // EDITOR STATE
        GEditorState globalEditorState;


        // INPUT MANAGEMENT
        // Create input manager object
        ale::InputManager input;
        input.init(renderer.getWindow());


        // TODO: The following stuff is here for testing, move it
        // as quickly as you find a proper place for it

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

            mainCam->setPos(glm::vec3(0, middleY, -frontEdge));
        } else {
            mainCam->setPos(glm::vec3(0, 50, 150));
        }

        mainCam->setSpeed(1.0f);

        // Further code tests camera movement
        // TODO: move this code from the main file,
        // provide better handling for bindings

        float cameraSpeedAdjusted = 1.0f;

        float mouseSensitivity = 0.06f;

		// Movement along global Y aixs
        auto moveY  = [&]() {mainCam->movePosGlobal( glm::vec3(0,1,0), cameraSpeedAdjusted);};
        auto moveNY = [&]() {mainCam->movePosGlobal(glm::vec3(0,-1,0), cameraSpeedAdjusted);};

		// WASD free camera movement
        auto moveF = [&]() { mainCam->moveForwardLocal(cameraSpeedAdjusted);};
        auto moveB = [&]() {mainCam->moveBackwardLocal(cameraSpeedAdjusted);};
        auto moveL = [&]() {    mainCam->moveLeftLocal(cameraSpeedAdjusted);};
        auto moveR = [&]() {   mainCam->moveRightLocal(cameraSpeedAdjusted);};


        ale::geo::REMesh reMesh;
        loader.populateREMesh(model.meshes[0], reMesh);

        // Removes extra digits after comma in floats
        auto r = [](float x) {
            // 1000 -> 3 digits
            const int DIGITS = 1000;
            return floorf(x * DIGITS) / DIGITS;
        };


        // Test function
        // Selects a triangle in the middle of the screen and
        // adds it to the ui draw buffer
        auto raycast = [&]() {
            bool result = false;
            glm::vec2 out_intersection_point = glm::vec2(0);

            float distance = -1;
            for (auto f : reMesh.faces) {
                result = geo::rayIntersectsTriangle(mainCam->getPos(),
                                                    mainCam->getForwardVec(),
                                                    f,
                                                    out_intersection_point,
                                                    distance);
                if (result) {
                    std::vector<geo::Loop*> out_loops = {};
                    geo::getBoundingLoops(f, out_loops);
                    std::vector<glm::vec3> loopVec = {};
                    loopVec.resize(3);

                    for (auto l : out_loops) {
                        loopVec.push_back(l->v->pos);
                    }

                    renderer.pushToUIDrawQueue({loopVec,ale::VERT});
                    break;
                }
            }

            ale::Tracer::raw << "Raycast result: " << result << "\n";
            ale::Tracer::raw << "Distance: " << distance << "\n";
        };

        // Removes all primitives from the buffer
        auto flushBuffer = [&](){
            renderer.flushUIDrawQueue();
        };

        auto changeModeEditor = [&](){
            globalEditorState.setNextModeTransform();
            trc::log("Editor mode changed", trc::DEBUG);
        };

        auto changeModeOperation = [&](){
            globalEditorState.setNextModeTransform();
            trc::log("Operation mode changed", trc::DEBUG);
        };

        // Bind lambda functions to keyboard actions
        using inp = ale::InputAction;
        input.bindFunction(inp::CAMERA_MOVE_F, moveF, true);
        input.bindFunction(inp::CAMERA_MOVE_B, moveB, true);
        input.bindFunction(inp::CAMERA_MOVE_L, moveL, true);
        input.bindFunction(inp::CAMERA_MOVE_R, moveR, true);
        input.bindFunction(inp::CAMERA_MOVE_U, moveY, true);
        input.bindFunction(inp::CAMERA_MOVE_D,moveNY, true);
        input.bindFunction(inp::ADD_SELECT,raycast, false);
        input.bindFunction(inp::RMV_SELECT_ALL,flushBuffer, false);
        input.bindFunction(inp::CYCLE_MODE_EDITOR,changeModeEditor, false);
        input.bindFunction(inp::CYCLE_MODE_OPERATION,changeModeOperation, false);

        // Bind global camera to the inner camera object
        renderer.bindCamera(mainCam);
        renderer.initRenderer();


        // Arbitrary ui events to execute
        std::function<void()> uiEvents = [&](){

            auto ubo = renderer.getUbo();
            using ui = ale::UIManager;

            /// GIZMOS START
            // The id of the first node containing a mesh
            // TODO: Implement proper node selection
            int id = 0;
            for (auto n : model.nodes) {
                if (n.mesh > -1) {
                    id = n.id;
                    break;
                }
            }
            // Manipulate a node with an ImGui Gizmo
            ui::drawImGuiGizmo(ubo.view, ubo.proj, model.nodes[id].transform, globalEditorState);
            /// GIZMOS FINISH

        };

        // MAIN RENDERING LOOP
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

            mainCam->setOrientation(camYawPitch.x,camYawPitch.y);

            // Drawing the results of the input
            renderer.drawFrame(uiEvents);

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
