
#pragma once
#ifndef ALE_APP
#define ALE_APP

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


namespace ale {

struct AppConfigData {
    // Command line arguments
    int argc;
    char **argv;
};

class App {

private:
    AppConfigData _config;
public:
    App(AppConfigData config);
    int run();
};

} // namespace ale

#endif //ALE_APP