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

// An action executed by the editor
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
    // NOTE: [03.12.2023] I use `map` instead of `unordered_map` for better
    // traversal time complexity and cache coherency.
    // It is possible to use a `vector` to store keys of the `unordered_map`, 
    // but now readability is more important than O(1)

    // A map of (KEY,ACTION) bindings
    std::map<int, InputAction> _inputBindings;
    // A map for key shortcuts and text inputs (uses callbacks)
    std::map<InputAction, std::function<void()> > _callbackBindings;
    // A map for continuous input (uses runtime checks)
    std::map<InputAction, std::function<void()> > _functionContBindings;

    // TODO: This is a raw pointer. Get rid of it later
    GLFWwindow* window_p;
    
    void _bindKey(int key, InputAction action);
    static void _keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    void _executeAction(InputAction _action);
    bool _isKeyPressed(GLFWwindow *window, int key);

public:
    InputManager();
    ~InputManager();
    void init(GLFWwindow *window);
    bool executeActiveActions();
    void bindFunction(InputAction _action, std::function<void()> _function, bool isContinuous = false);

    bool isActionActive(InputAction _action);
};

InputManager::InputManager() {}

InputManager::~InputManager() {}

// TODO: It is probably a good idea to move this to the constructor
void InputManager::init(GLFWwindow* window) {
    window_p = window;
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
    if (_callbackBindings.contains(_action)) {
		_callbackBindings[_action]();
    }
}

void InputManager::_bindKey(int key, InputAction action) {
    _inputBindings[key] = action;
}

bool InputManager::_isKeyPressed(GLFWwindow* window, int key){
    return glfwGetKey(window, key);
}

// Binds a lambda funciton to an editor action. For now the function should have
// a declaration `void func_name(void)`, but it can use context data
// TODO: implement a more verbose distinction between input methods (i.e. enum)
void InputManager::bindFunction(InputAction _action, std::function<void()> _function, bool isContinuous) {    
    if (isContinuous) {
        _functionContBindings[_action] = _function;
    } else {
        _callbackBindings[_action] = _function;        
    }
}

bool InputManager::isActionActive(InputAction _action){
    return false;
} 

// Allows to execute active continuous actions (like camera controls)
// Should be called from the main loop/input thread
bool InputManager::executeActiveActions() {
    for (auto binding:_inputBindings) {
        if (_isKeyPressed(window_p, binding.first)) {
            if (_functionContBindings[binding.second]) {
                _functionContBindings[binding.second]();
            } else {
                return false;
            }
        }
    }
    return true;
}
    
} // namespace ale

#endif //ALE_INPUT_MANAGER