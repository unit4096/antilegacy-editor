#include "app.h"


App::App(AppConfigData config) {
    this->_config = config;
}


/*
    Runs the editor. It connects every other class
    whether it is window creation or input management.
    As of [14.01.2024] it is intentionally verboose
    for the ease of development

    There are four main classes in the editor loop:
    1) GEditorState -- state of the editor: selections, actions, modes
    2) InputManager -- handles GLFW input and dispatches it to actions
    3) EventManager -- binds actions to event lambdas that modify editor state
    4) Renderer -- draws the results and provides immediate mode UI

*/

int App::run() {

    try {
        // FRAMERATE MANAGEMENT
        // FPS cap in 8 milliseconds -> 120 FPS
        const std::chrono::duration<double, std::milli> fps_cap(8);
        auto lastFrame = std::chrono::steady_clock::now();
        std::chrono::duration<double> deltaTime;

        trc::raw << "\n\n";

        // LOGGING (Enables all logging levels by default)
        // std::vector<trc::LogLevel> logLevels = { trc::LogLevel::DEBUG, trc::LogLevel::INFO, trc::LogLevel::WARNING, trc::LogLevel::ERROR,};
        // trc::SetLogLevels(logLevels);

        // MODEL LOADING
        // Fox.gltf is a default model to load when no path provided
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
            return -1;
        }

        // EDITOR STATE
        sp<GEditorState> state = std::make_shared<GEditorState>();


        state->currentREMesh = &model.reMeshes[0];
        state->currentModel = &model;
        state->currentModelNode = nullptr;

        // RENDERING
        // Create Vulkan renderer
        ale::Renderer ren(model);
        sp<ale::Renderer> renderer = std::make_shared<ale::Renderer>(ren);
        renderer->initWindow();

        // Create a camera object that will be passed to the renderer
        sp<ale::Camera> mainCam = std::make_shared<ale::Camera>();
        renderer->bindCamera(mainCam);

        mainCam->setSpeed(1.0f);
        mainCam->setSensitivity(0.06f);
        // Dynamically set camera position by mesh bounding box
        assert(model.viewMeshes.size() > 0);
        // Uses AABB of the first mesh
        if (model.viewMeshes[0].minPos.size() >= 3) {
            mainCam->setPos(ale::geo::getFrontViewAABB(model.viewMeshes[0]));
        } else {
            mainCam->setPos(glm::vec3(0, 50, 150));
        }


        // INPUT MANAGEMENT
        // Create input manager object
        ale::InputManager inp;
        sp<InputManager> input = std::make_shared<InputManager>(inp);
        // Bind input to window
        input->init(renderer->getWindow());


        // EVENT MANAGEMENT
        ale::EventManager eventManager(renderer,state,input);

        // Start renderer
        renderer->initRenderer();


        // Arbitrary ui events to execute

        // MAIN RENDERING LOOP
        while (!renderer->shouldClose()) {
            // Time point to the frame start
            auto thisFrame = std::chrono::steady_clock::now();
            // Delta time for editor calculations
            deltaTime = duration_cast<std::chrono::duration<double>>(thisFrame - lastFrame);
            lastFrame = thisFrame;

            sp<Camera> _cam = renderer->getCurrentCamera();

            _cam->setDelta(deltaTime.count() * 100 / 8);

            // polling events, callbacks fired
            glfwPollEvents();
            // Key camera input
            input->executeActiveKeyActions();

            // Mouse camera input
            input->executeActiveMouseAcitons();
            auto mouseMovement = input->getLastDeltaMouseOffset();
            auto camYawPitch = _cam->getYawPitch();

            camYawPitch-= mouseMovement * _cam->getSensitivity();

            _cam->setOrientation(camYawPitch.x,camYawPitch.y);


            std::function<void()> perFrameUIEvents = [&] () {
                eventManager.frameEventCallback();
            };

            // Drawing the results of the input
            renderer->drawFrame(perFrameUIEvents);

            // Smooth framerate
            auto thisFrameEnd = std::chrono::steady_clock::now();
            auto frameDuration = duration_cast<std::chrono::duration<double>>(thisFrameEnd - thisFrame);

            // Sleep for = cap time - frame duration (to avoid FPS spikes)
            const auto remainder = fps_cap - frameDuration;
            // NOTE: on some hardware enabled VSync will slow down the
            // system to sleep_for + VSync wait. In this case just
            // disable this fps cap
            /* std::this_thread::sleep_for(remainder); */

        }
        renderer->cleanup();

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
