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
    static glm::vec2 worldToScreen(const glm::mat4& modelViewProjection, const glm::vec3& pos, const glm::vec2& screenSize);
    static void drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2, const glm::mat4& mvp, glm::vec2 screenSize);
};


} // namespace ale

#endif // ALE_UI_MANAGER
