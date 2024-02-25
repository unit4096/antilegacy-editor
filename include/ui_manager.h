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
    static void DrawUiBg();
    static glm::vec2 worldToScreen(const glm::mat4& modelViewProjection, const glm::vec3& pos);
    static void drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2, const glm::mat4& mvp);
    static void drawWorldSpaceVert(const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3, const glm::mat4& mvp);
    static void drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model);
    static void flipProjection(glm::mat4& proj);
    static glm::mat4 getFlippedProjection(const glm::mat4& proj);
};


} // namespace ale

#endif // ALE_UI_MANAGER
