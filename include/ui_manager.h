#pragma once

#ifndef ALE_UI_MANAGER
#define ALE_UI_MANAGER

// ext
#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#endif // GLM
#include <vector>


// int
#include <ale_imgui_interface.h>
#include <primitives.h>
#include <tracer.h>
#include <ale_geo_utils.h>
#include <camera.h>
#include <memory.h>
#include <editor_state.h>


namespace ale {



class UIManager {
public:
    static glm::vec2 worldToScreen(const glm::mat4& modelViewProjection, const glm::vec3& pos);
    static void flipProjection(glm::mat4& proj);
    static glm::mat4 getFlippedProjection(const glm::mat4& proj);
    static ale::MVP getMVPWithFlippedProjection(const MVP& mvp);
    static void vec3Handler(glm::vec3& vec, float dnLim, float upLim);
    static void vec3Handler(std::vector<float>& vec, float dnLim, float upLim);
    static void drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2, const MVP& mvpMat);
    static void drawWorldSpaceVert(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3, const MVP& mvpMat);
    static void drawWorldSpaceCircle(const glm::vec3& pos, const MVP& mvp);
    static void drawVectorOfPrimitives(const std::vector<glm::vec3>& vec, UI_DRAW_TYPE mode, const MVP& pvm);
    static void drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model, GEditorState& state);
    static void drawNodeRootsUI(const ale::Model& model, const MVP& pvm);
    static void drawMenuBarUI();
    static void drawHierarchyUI(const ale::Model& model);
    static void CameraControlWidgetUI(sp<ale::Camera> cam);
    static void drawDefaultWindowUI(sp<ale::Camera> cam, const ale::Model& model, MVP pvm);
    static void drawAABB(const glm::vec3& min, const glm::vec3& max, const MVP& mvp);
    static void drawRaycast(const glm::vec3& pos, const glm::vec3 dir,float length,  const MVP& mvp);
};


} // namespace ale

#endif // ALE_UI_MANAGER
