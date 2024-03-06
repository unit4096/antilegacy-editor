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
};


} // namespace ale

#endif // ALE_UI_MANAGER
