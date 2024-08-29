#ifndef ALE_INPUT_MANAGER
#define ALE_INPUT_MANAGER


// ext
#pragma once
#include <functional>
#include <map>

#ifndef GLFW
#define GLFW
#define GLFW_INCLUDE_VULKAN
#define GLFW_CURSOR_DISABLED
#include <GLFW/glfw3.h>
#endif //GLFW

// int

#include <primitives.h>
#include <tracer.h>
#include <ale_imgui_interface.h>

namespace trc = ale::Tracer;

namespace ale {




// An action executed by the editor
// NOTE: Some action should be recorded by an undo system, while some
// should not. I do not know yet whether i should separate such actions
// into separate enums
enum InputAction {

    // Free camera movement
    CAMERA_MOVE_F,
    CAMERA_MOVE_B,
    CAMERA_MOVE_L,
    CAMERA_MOVE_R,
    CAMERA_MOVE_U,
    CAMERA_MOVE_D,

    // Add and remove selections
    ADD_SELECT,
    RMV_SELECT_ALL,

    // Editor mode cycling
    CYCLE_MODE_PRIMITIVE,
    CYCLE_MODE_EDITOR,
    CYCLE_MODE_SPACE,
    CYCLE_MODE_OPERATION,

    // Set specific primitive mode
    SET_MODE_PRIMITIVE_V,
    SET_MODE_PRIMITIVE_E,
    SET_MODE_PRIMITIVE_F,


    SET_MODE_EDITOR_OBJ,
    SET_MODE_EDITOR_MESH,
    SET_MODE_EDITOR_UV,
    SET_MODE_EDITOR_ANIM,
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
    void setActionBinding(InputAction _action, std::function<void()> _function, bool isContinuous = false);
    void clearActionBinding(InputAction _action);
    bool isActionActive(InputAction _action);
    glm::highp_vec2 getLastDeltaMouseOffset();
    glm::highp_vec2 getMousePos();
};

} // namespace ale

#endif //ALE_INPUT_MANAGER
