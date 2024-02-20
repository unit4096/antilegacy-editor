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
    FUNC_1,
    FUNC_2,
    FUNC_3,
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
    GLFWwindow* _window_p;
    
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

    
} // namespace ale

#endif //ALE_INPUT_MANAGER
