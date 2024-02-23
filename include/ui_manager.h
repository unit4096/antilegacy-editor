#pragma once

#ifndef ALE_UI_MANAGER
#define ALE_UI_MANAGER

#ifndef GLM
#define GLM
// ext
#include <glm/glm.hpp>
#endif // GLM

// int
#include <ale_imgui_interface.h>
#include <primitives.h>
#include <vector>

#include <tracer.h>


namespace ale {

class UIManager {
public:
    static void DrawUiBg();
    static glm::vec2 worldToScreen(const glm::mat4& modelViewProjection, const glm::vec3& pos);
    static void drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2, const glm::mat4& mvp);
    static void drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model);
    static void flipProjection(glm::mat4& proj);

};


} // namespace ale

#endif // ALE_UI_MANAGER
