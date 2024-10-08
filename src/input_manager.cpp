#include <input_manager.h>


InputManager::InputManager() {}

InputManager::~InputManager() {}

// TODO: It is probably a good idea to move this to the constructor
void InputManager::init(GLFWwindow* window) {
    _window_p = window;
    glfwSetWindowUserPointer(window, this);
    trc::log("Init input manager", trc::LogLevel::INFO);

    // TODO: load config from a TOML file, add wrappers for raw GLFW macros
	_bindKey(GLFW_KEY_W,InputAction::CAMERA_MOVE_F);
	_bindKey(GLFW_KEY_S,InputAction::CAMERA_MOVE_B);
	_bindKey(GLFW_KEY_A,InputAction::CAMERA_MOVE_L);
	_bindKey(GLFW_KEY_D,InputAction::CAMERA_MOVE_R);
	_bindKey(GLFW_KEY_Q,InputAction::CAMERA_MOVE_U);
	_bindKey(GLFW_KEY_E,InputAction::CAMERA_MOVE_D);
	_bindKey(GLFW_KEY_R,InputAction::ADD_SELECT);

	_bindKey(GLFW_KEY_F,InputAction::RMV_SELECT_ALL);
	_bindKey(GLFW_KEY_C,InputAction::CYCLE_MODE_OPERATION);
	_bindKey(GLFW_KEY_1,InputAction::CYCLE_MODE_EDITOR);

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
void InputManager::setActionBinding(InputAction _action, std::function<void()> _function, bool isContinuous) {
    if (isContinuous) {
        _functionContBindings[_action] = _function;
    } else {
        _callbackBindings[_action] = _function;
    }
}


void InputManager::clearActionBinding(InputAction _action) {
    _functionContBindings[_action] = nullptr;
}


// FIXME: Implement this function or delete the declaration
bool InputManager::isActionActive(InputAction _action){
    trc::log("Not implemented!", trc::ERROR);
    return false;
}

// Allows to execute active continuous actions (like camera controls)
// Should be called from the main loop/input thread
bool InputManager::executeActiveKeyActions() {
    for (auto binding:_inputKeyBindings) {
        if (_isKeyPressed(_window_p, binding.first)) {
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
glm::highp_vec2 InputManager::getLastDeltaMouseOffset() {
    return {_lastDeltaX, _lastDeltaY};
}

glm::highp_vec2 InputManager::getMousePos() {
    return {_lastPosX, _lastPosY};
}

// Executes mouse actions
// TODO: add mouse mode checks, enable only when over the model window
bool InputManager::executeActiveMouseAcitons() {

    auto io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return true;
    }


    double xpos, ypos;

    int state = glfwGetMouseButton(_window_p, GLFW_MOUSE_BUTTON_LEFT);

    _lastDeltaX = 0;
    _lastDeltaY = 0;

    glfwGetCursorPos(_window_p, &xpos, &ypos);

    // Calculates the delta offset for mouse movement
    if (state == GLFW_PRESS) {
        if (_lastPosX != xpos || _lastPosY != ypos) {
            if (_isLastPressed){
                _lastDeltaX = xpos - _lastPosX;
                _lastDeltaY = ypos - _lastPosY;
            }
        }
        _isLastPressed = true;
    } else {
        _isLastPressed = false;
    }


    _lastPosX = xpos;
    _lastPosY = ypos;
    return true;
}
