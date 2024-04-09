/*
   This class handles editor events, both as input and history events


FIXME: Currently it seeems like  class has to many
responsibilities. Think about better design patterns
*/

#pragma once
#ifndef ALE_EVENT_MANAGER
#define ALE_EVENT_MANAGER


//ext
#include <functional>

//int
#include <tracer.h>
#include <memory.h>
#include <input_manager.h>
#include <ui_manager.h>
#include <os_loader.h>
#include <camera.h>
#include <editor_state.h>
#include <ale_geo_utils.h>
#include <re_mesh.h>
#include <renderer.h>


namespace ale {

class EventManager {
public:
    EventManager(sp<ale::Renderer> renderer,
                 sp<ale::GEditorState> editorState,
                 sp<ale::InputManager> inputManager) {

        _renderer = renderer;
        _editorState = editorState;
        _inputManager = inputManager;


        // Bind lambda functions to keyboard actions
        using inp = ale::InputAction;
        _inputManager->bindFunction(inp::CAMERA_MOVE_F, moveF, true);
        _inputManager->bindFunction(inp::CAMERA_MOVE_B, moveB, true);
        _inputManager->bindFunction(inp::CAMERA_MOVE_L, moveL, true);
        _inputManager->bindFunction(inp::CAMERA_MOVE_R, moveR, true);
        _inputManager->bindFunction(inp::CAMERA_MOVE_U, moveY, true);
        _inputManager->bindFunction(inp::CAMERA_MOVE_D,moveNY, true);


        _inputManager->bindFunction(inp::ADD_SELECT,raycast, false);
        _inputManager->bindFunction(inp::RMV_SELECT_ALL,flushBuffer, false);
        _inputManager->bindFunction(inp::CYCLE_MODE_EDITOR,changeModeEditor, false);
        _inputManager->bindFunction(inp::CYCLE_MODE_OPERATION,changeModeOperation, false);

    }

private:
    sp<ale::Renderer> _renderer;
    sp<ale::GEditorState> _editorState;
    sp<ale::InputManager> _inputManager;

    // Movement along global Y aixs
    std::function<void()> moveY  = [&]() {_renderer->getCurrentCamera()->movePosGlobal( glm::vec3(0,1,0));};
    std::function<void()> moveNY = [&]() {_renderer->getCurrentCamera()->movePosGlobal(glm::vec3(0,-1,0));};

    // WASD free camera movement
    std::function<void()> moveF = [&]() { _renderer->getCurrentCamera()->moveForwardLocal();};
    std::function<void()> moveB = [&]() {_renderer->getCurrentCamera()->moveBackwardLocal();};
    std::function<void()> moveL = [&]() {    _renderer->getCurrentCamera()->moveLeftLocal();};
    std::function<void()> moveR = [&]() {   _renderer->getCurrentCamera()->moveRightLocal();};

    // Test function
    // Selects a triangle in the middle of the screen and
    // adds it to the ui draw buffer
    std::function<void()> raycast = [&]() {

        if (!_editorState->currentREMesh) {
            trc::log("Current REMesh is null", trc::WARNING);
            return;
        }

        auto ubo = _renderer->getUbo();
        auto mouse = _inputManager->getMousePos();
        auto displaySize = _renderer->getDisplaySize();

        auto pos = _renderer->getCurrentCamera()->getPos();
        auto fwd = geo::screenToWorld(
            ale::UIManager::getFlippedProjection(ubo.proj)
            * ubo.view, {mouse.x,mouse.y}, displaySize);


        bool result = false;
        glm::vec2 intersection = glm::vec2(0);
        float distance = -1;
        for (auto& f : _editorState->currentREMesh->faces) {
            result = geo::rayIntersectsTriangle(pos, fwd, f, intersection, distance);
            if (result) {
                std::vector<geo::Loop*> out_loops = {};
                geo::getBoundingLoops(f, out_loops);
                std::vector<glm::vec3> loopVec = {};
                loopVec.reserve(3);

                for (auto& l : out_loops) {
                    loopVec.push_back(l->v->pos);
                }

                _renderer->pushToUIDrawQueue({loopVec,ale::VERT});
                break;
            }
        }

        std::string msg = "Raycast result: " + std::to_string(result);
        msg.append(" Distance: " + std::to_string(distance));
        trc::log(msg);
    };

    // Removes all primitives from the buffer
    std::function<void()> flushBuffer = [&](){
        _renderer->flushUIDrawQueue();
    };

    std::function<void()> changeModeEditor = [&](){
        _editorState->setNextModeEditor();
        trc::log("Editor mode changed", trc::DEBUG);
    };

    std::function<void()> changeModeOperation = [&](){
        _editorState->setNextModeTransform();
        trc::log("Operation mode changed to " + std::to_string(_editorState->transformMode), trc::DEBUG);
    };
};

} // namespace ale


#endif //ALE_EVENT_MANAGER
