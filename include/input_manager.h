#ifndef ALE_INPUT_MANAGER
#define ALE_INPUT_MANAGER


// ext
#pragma once
#include <iostream>
#include <functional>
#include <map>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif //GLFW

namespace ale {

enum InputAction {
    CAMERA_MOVE_F,
    CAMERA_MOVE_B,
    CAMERA_MOVE_L,
    CAMERA_MOVE_R,
    CAMERA_MOVE_U,
    CAMERA_MOVE_D,
};

class InputManager {
private:
    // map of bindigns to actions (not an unordered_map for better traversal)
    std::map<int, InputAction> _inputBindings; 
    std::map<InputAction, std::function<void()> > _functionBindings;
    void _bindKey(int key, InputAction action);
    static void _keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    void _executeAction(InputAction _action);

    

public:
    InputManager();
    ~InputManager();
    void init(GLFWwindow *window);   
    void bindFunction(InputAction _action, std::function<void()> _function);
};

InputManager::InputManager() {}

InputManager::~InputManager() {}

void InputManager::init(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);
    std::cout << "INIT INPUT MANAGER"<< std::endl;

    // TODO: load config from a TOML file, add wrappers for raw GLFW macros
	_bindKey(GLFW_KEY_W,InputAction::CAMERA_MOVE_F);
	_bindKey(GLFW_KEY_S,InputAction::CAMERA_MOVE_B);
	_bindKey(GLFW_KEY_A,InputAction::CAMERA_MOVE_L);
	_bindKey(GLFW_KEY_D,InputAction::CAMERA_MOVE_R);
	_bindKey(GLFW_KEY_Q,InputAction::CAMERA_MOVE_U);
	_bindKey(GLFW_KEY_Z,InputAction::CAMERA_MOVE_D);

    glfwSetKeyCallback(window, _keyCallback);
}

void InputManager::_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* manager = (InputManager*)glfwGetWindowUserPointer(window);
    
    if (manager->_inputBindings.contains(key) && action == GLFW_PRESS) {
        auto _action = manager->_inputBindings[key];
        manager->_executeAction(_action);
    }
}

void InputManager::_executeAction(InputAction _action) {
    if (_functionBindings.contains(_action)) {
		_functionBindings[_action]();
    } else {
		std::cout << "ALE: Function not found for action!\n";
	}
    // std::cout << "Action called: " << _action << "\n";
}

void InputManager::_bindKey(int key, InputAction action) {
    _inputBindings[key] = action;
}

void InputManager::bindFunction(InputAction _action, std::function<void()> _function) {
    _functionBindings[_action] = _function;    
}
    
} // namespace ale

#endif //ALE_INPUT_MANAGER