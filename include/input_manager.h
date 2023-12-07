#ifndef ALE_INPUT_MANAGER
#define ALE_INPUT_MANAGER


// ext
#pragma once
#include <functional>
#include <map>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif //GLFW

// int

#include <primitives.h>
#include <tracer.h>

namespace trc = ale::Tracer;

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
    std::map<int, InputAction> _inputKeyBindings;
    // A map for key shortcuts and text inputs (uses callbacks)
    std::map<InputAction, std::function<void()> > _callbackBindings;
    // A map for continuous input (uses runtime checks)
    std::map<InputAction, std::function<void()> > _functionContBindings;

    double _lastPosX, _lastPosY;
    double _lastDeltaX = 0.0, _lastDeltaY = 0.0;
    // 
    bool _isLastPressed = false;

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
    bool executeActiveKeyActions();
    bool executeActiveMouseAcitons();
    void bindFunction(InputAction _action, std::function<void()> _function, bool isContinuous = false);
    bool isActionActive(InputAction _action);
    v2d getLastDeltaMouseOffset();
};

InputManager::InputManager() {}

InputManager::~InputManager() {}

// TODO: It is probably a good idea to move this to the constructor
void InputManager::init(GLFWwindow* window) {
    window_p = window;
    glfwSetWindowUserPointer(window, this);
    trc::log("INIT INPUT MANAGER", trc::LogLevel::INFO);

    // TODO: load config from a TOML file, add wrappers for raw GLFW macros
	_bindKey(GLFW_KEY_W,InputAction::CAMERA_MOVE_F);
	_bindKey(GLFW_KEY_S,InputAction::CAMERA_MOVE_B);
	_bindKey(GLFW_KEY_A,InputAction::CAMERA_MOVE_L);
	_bindKey(GLFW_KEY_D,InputAction::CAMERA_MOVE_R);
	_bindKey(GLFW_KEY_Q,InputAction::CAMERA_MOVE_U);
	_bindKey(GLFW_KEY_Z,InputAction::CAMERA_MOVE_D);

    glfwSetKeyCallback(window, _keyCallback);
}

// types: GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
void InputManager::_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* manager = (InputManager*)glfwGetWindowUserPointer(window);
    
    if (manager->_inputKeyBindings.contains(key) && action == GLFW_PRESS) {
        auto _action = manager->_inputKeyBindings[key];
        manager->_executeAction(_action);
    }
}

void InputManager::_executeAction(InputAction _action) {
    if (_callbackBindings.contains(_action)) {
		_callbackBindings[_action]();
    }
}

void InputManager::_bindKey(int key, InputAction action) {
    _inputKeyBindings[key] = action;
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
bool InputManager::executeActiveKeyActions() {
    for (auto binding:_inputKeyBindings) {
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

// Get the last recorded delta offset of the mouse (last movement)
// x -- horizontal movement
// y -- vertical movement
v2d InputManager::getLastDeltaMouseOffset() {
    return v2d(_lastDeltaX,_lastDeltaY);
}

// Executes mouse actions
// TODO: add mouse mode checks, enable only when over the model window
bool InputManager::executeActiveMouseAcitons() {
    double xpos, ypos;
    
    int state = glfwGetMouseButton(window_p, GLFW_MOUSE_BUTTON_LEFT);

    _lastDeltaX = 0;
    _lastDeltaY = 0;
    
    // Calculates the delta offset for mouse movement
    if (state == GLFW_PRESS) {
        glfwGetCursorPos(window_p, &xpos, &ypos);
        if (_lastPosX != xpos || _lastPosY != ypos) {
            if (_isLastPressed){
                _lastDeltaX = xpos - _lastPosX;
                _lastDeltaY = ypos - _lastPosY;
            }
            _lastPosX = xpos;
            _lastPosY = ypos;
        }
        _isLastPressed = true;
    } else {
        _isLastPressed = false;
    }
    return true;
}
    
} // namespace ale

#endif //ALE_INPUT_MANAGER