#ifndef ALE_INPUT
#define ALE_INPUT

#pragma once

#include <iostream>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif //GLFW

namespace ale {

class InputManager {
private:
        
public:
    InputManager();
    ~InputManager();
    void init(GLFWwindow *window);
};

InputManager::InputManager() {}

InputManager::~InputManager() {}

void InputManager::init(GLFWwindow* window) {
    std::cout << "PRESS E TO CHECK INPUT"<< std::endl;
        
    auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_E && action == GLFW_PRESS) {
            std::cout << "KEY E CALLBACK" << std::endl;
        }
    };
    glfwSetKeyCallback(window, key_callback);
}
    
} // namespace ale

#endif //ALE_INPUT