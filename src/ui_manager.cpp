#include <ui_manager.h>

using namespace ale;

void UIManager::DrawUiBg() {

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    ImVec2 p3(600,100);
    ImVec2 p4(1000,1000);
    listBg->AddLine(p3, p4, IM_COL32(255, 255, 255, 255), 5);

}

void UIManager::flipProjection(glm::mat4& proj) {
        proj *=  glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
}

glm::vec2 UIManager::worldToScreen(const glm::mat4& modelViewProjection,
                        const glm::vec3& pos) {

    ImGuiIO& io = ImGui::GetIO();
    // Transform world position to clip space
    glm::vec4 clipCoords = modelViewProjection * glm::vec4(pos,1.0f);

    // Convert clip space coordinates to NDC (-1 to 1)
    clipCoords /= clipCoords.w;

    // Convert NDC coordinates to screen space pixel coordinates
    float screenX = (clipCoords.x + 1.0f) * 0.5f * io.DisplaySize.x;
    float screenY = (1.0f - clipCoords.y) * 0.5f * io.DisplaySize.y;

    return glm::vec2(screenX, screenY);
}

void UIManager::drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2,
                                   const glm::mat4& mvp) {
    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = worldToScreen(mvp, pos1);
    glm::vec2 screenPos2 = worldToScreen(mvp, pos2);
    ImVec2 p1(screenPos1.x, screenPos1.y);
    ImVec2 p2(screenPos2.x, screenPos2.y);
    listBg->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 5);
}

static void drawImGuiGrid(){
    ale::Tracer::log("Not implemented!", ale::Tracer::ERROR);
}

void UIManager::drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model){
    UIManager::flipProjection(proj);
    float* _view = geo::glmMatToPtr(view);
    float* _proj = geo::glmMatToPtr(proj);
    float* _model = geo::glmMatToPtr(model);

    auto enum_translate = ImGuizmo::OPERATION::TRANSLATE;
    auto enum_worldspace = ImGuizmo::MODE::WORLD;

    ImGuizmo::Manipulate(_view, _proj, enum_translate, enum_worldspace, _model);
}

void drawCoreUI() {


    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Test Item", "TEST*BINDING")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Test Item 2", "TEST+BINDING+2")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }



}
