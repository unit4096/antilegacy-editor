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


namespace ale {


// Types of primitives to draw
enum UI_DRAW_TYPE {
    LINE,
    VERT,
    CIRCLE,
    UI_DRAW_TYPE_MAX,
};

class UIManager {
public:
    static glm::vec2 worldToScreen(const glm::mat4& modelViewProjection, const glm::vec3& pos);
    static void drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2, const MVP& mvpMat);
    static void drawWorldSpaceVert(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3, const MVP& mvpMat);
    static void drawWorldSpaceCircle(const glm::vec3& pos, const MVP& mvp);
    static void drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model);
    static void flipProjection(glm::mat4& proj);
    static glm::mat4 getFlippedProjection(const glm::mat4& proj);
    static void drawMenuBar();
    static void vec3Handler(glm::vec3& vec, float dnLim, float upLim);
    static void drawHierarchyUI(const ale::Model& model);
    static void drawNodeRoots(const ale::Model& model, const MVP& pvm);
    static void drawVectorOfPrimitives(const std::vector<glm::vec3>& vec, UI_DRAW_TYPE mode, const MVP& pvm);
};


} // namespace ale

#endif // ALE_UI_MANAGER
