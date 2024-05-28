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

        using inp = ale::InputAction;
        _renderer = renderer;
        _editorState = editorState;
        _inputManager = inputManager;

        // Bind lambda functions to keyboard actions
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

void frameEventCallback() {
    using ui = ale::UIManager;
    auto ubo = this->_renderer->getUbo();
    auto& _state = this->_editorState;

    MVP pvm = {.m = ubo.model, .v = ubo.view, .p = ui::getFlippedProjection(ubo.proj)};
    if (_state->currentModelNode && _state->editorMode == ale::OBJECT_MODE) {
        ui::drawImGuiGizmo(ubo.view, ubo.proj, &_state->currentModelNode->transform , *_state.get());
    } else if (!_state->selectedFaces.empty() && _state->editorMode == ale::MESH_MODE) {
        // Get first vertice
        auto& v = _state->selectedFaces[0]->loop->v;

        auto tr = geo::constructTransformFromPos(v->pos);

        ui::drawImGuiGizmo(ubo.view, ubo.proj, &tr, *_state.get());

        auto newPos = geo::extractPosFromTransform(tr);
        v->pos = newPos;

    }

    for(auto pair: _state->uiDrawQueue) {
        ale::UI_DRAW_TYPE type = pair.second;
        auto vec = pair.first;

        auto parentPos = geo::extractPosFromTransform(_state->currentModelNode->transform);

        for(auto& e : vec) {
            e += parentPos;
        }

        ui::drawVectorOfPrimitives(vec, type, pvm);
    }

    std::string msg = "selected node: " + (_state->currentModelNode == nullptr
                      ? "NONE"
                      : _editorState->currentModel->nodes[_state->currentModelNode->id].name);
    msg.append("\neditor mode: " + ale::GEditorMode_Names[_state->editorMode]);
    msg.append("\ntransfor mode: " + ale::GTransformMode_Names[_state->transformMode]);
    msg.append("\nspace mode: " + ale::GSpaceMode_Names[_state->spaceMode]);
    ui::drawTextBG({100,100}, msg);

};

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

    // FIXME: Without proper architecture this will get ugly pretty quickly
    std::function<void()> raycast = [&]() {

        auto ubo = _renderer->getUbo();
        auto mouse = _inputManager->getMousePos();
        auto displaySize = _renderer->getDisplaySize();

        auto pos = _renderer->getCurrentCamera()->getPos();
        auto fwd = geo::screenToWorld(
                   ale::UIManager::getFlippedProjection(ubo.proj)* ubo.view,
                   {mouse.x,mouse.y}, displaySize);

        switch (_editorState->editorMode) {
            case ale::MESH_MODE:
                raycastMeshMode(pos, fwd);
                break;

            case ale::OBJECT_MODE:
                raycastObjMode(pos, fwd);
                break;

            case ale::UV_MODE:
                trc::log("I DO STUFF in uv object mode");
                break;

            case ale::EDITOR_MODE_MAX:
                trc::log("Enum size passed as an argument!");
                break;
        }

        if (_editorState->currentModelNode == nullptr) {
            trc::log("No node selected!");
            return;
        }

        if (!_editorState->currentREMesh) {
            trc::log("Current REMesh is null", trc::WARNING);
            return;
        }
        /// START
    };


    // Removes all primitives from the buffer
    std::function<void()> flushBuffer = [this](){
        _editorState->uiDrawQueue.clear();
    };


    // TODO: Need a way to send a callback whenever the state changes
    std::function<void()> changeModeEditor = [this](){
        if (_editorState->editorMode != ale::OBJECT_MODE) {
            _editorState->editorMode = ale::OBJECT_MODE;
            trc::log("Editor mode is now OBJECT_MODE");
        } else {
            _editorState->editorMode = ale::MESH_MODE;
            trc::log("Editor mode is now MESH_MODE");
        }
    };


    std::function<void()> changeModeOperation = [&](){
        _editorState->setNextModeTransform();
        trc::log("Operation mode changed to " + ale::GTransformMode_Names[_editorState->transformMode], trc::DEBUG);
    };


    void raycastObjMode(const glm::vec3& pos, glm::vec3& fwd){
        bool _hits = false;

        for(auto& node : _editorState->currentModel->nodes) {
            if (node.mesh != -1) {

                auto& mesh = _editorState->currentModel->viewMeshes[node.mesh];
                auto aabb = geo::extractMinMaxAABB(mesh);
                auto pos4  = glm::inverse(node.transform) * glm::vec4(pos, 1.0f);

                if( geo::rayIntersectsAABB(pos4,fwd, aabb.first, aabb.second)) {
                    _hits = true;

                    _editorState->currentModelNode = &node;
                    _editorState->currentREMesh = &_editorState->currentModel->reMeshes[node.mesh];
                    _editorState->uiDrawQueue.push_back({{aabb.first, aabb.second},ale::AABB});
                }
            }
        }

        if (!_hits) {
            _editorState->currentModelNode = nullptr;
            _editorState->currentREMesh = nullptr;
        }
    };


    void raycastMeshMode(glm::vec3 pos, glm::vec3 fwd){

        if (!_editorState->currentModelNode) {
            trc::log("Current node is NULL", trc::WARNING);
            return;
        }

        bool result = false;
        glm::vec2 intersection = glm::vec2(0);
        float distance = -1;

        for (auto& f : _editorState->currentREMesh->faces) {
            auto pos4 = glm::vec4(pos, 1.0f);
            pos4 = glm::inverse(_editorState->currentModelNode->transform) * pos4;

            result = geo::rayIntersectsTriangle(pos4, fwd, f, intersection, distance);
            if (result) {

                std::vector<geo::Loop*> out_loops {};
                geo::getBoundingLoops(f, out_loops);
                std::vector<glm::vec3> loopVec {};
                loopVec.reserve(3);


                trc::raw << "\n face "<< trc::RED << f->id << trc::RESET << " face\n";

                for (auto& l : out_loops) {
                    auto _v = l->v;
                    trc::raw << "\n "<< _v->id << " vector \n";
                    loopVec.push_back(l->v->pos);
                }
                _editorState->uiDrawQueue.push_back({loopVec,ale::VERT});
                // TODO: Load range to selected buffer
                _editorState->selectedFaces.clear();
                _editorState->selectedFaces.push_back(f);
                break;
            }
        }

        std::string msg = "Raycast result: " + std::to_string(result);
        msg.append(" Distance: " + std::to_string(distance));
        trc::log(msg);
    };

};

} // namespace ale


#endif //ALE_EVENT_MANAGER
