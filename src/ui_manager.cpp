#include <ui_manager.h>

using namespace ale;

void UIManager::DrawUiBg() {

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    ImVec2 p3(600,100);
    ImVec2 p4(1000,1000);
    listBg->AddLine(p3, p4, IM_COL32(255, 255, 255, 255), 5);

}

glm::vec2 UIManager::worldToScreen(const glm::mat4& modelViewProjection,
                        const glm::vec3& pos,
                        const glm::vec2& screenSize) {
    // Transform world position to clip space
    glm::vec4 clipCoords = modelViewProjection * glm::vec4(pos,1.0f);

    // Convert clip space coordinates to NDC (-1 to 1)
    clipCoords /= clipCoords.w;

    // Convert NDC coordinates to screen space pixel coordinates
    float screenX = (clipCoords.x + 1.0f) * 0.5f * screenSize.x;
    float screenY = (1.0f - clipCoords.y) * 0.5f * screenSize.y;

    return glm::vec2(screenX, screenY);
}

void UIManager::drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2,
                                   const glm::mat4& mvp, glm::vec2 screenSize) {
    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = worldToScreen(mvp, pos1,screenSize);
    glm::vec2 screenPos2 = worldToScreen(mvp, pos2,screenSize);
    ImVec2 p1(screenPos1.x, screenPos1.y);
    ImVec2 p2(screenPos2.x, screenPos2.y);
    listBg->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 5);
}
