#include <ui_manager.h>


using namespace ale;

const float VIEW_LIMIT = VIEW_LIMIT;


// Flips passed projection
void UIManager::flipProjection(glm::mat4& proj) {
        proj *=  glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
}


// Returns a flipped version of the projection
glm::mat4 UIManager::getFlippedProjection(const glm::mat4& proj) {
        return proj * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
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


// A simple function that indicates if a point is "in front" enough
// of the camera. Used mostly to avoid inversing geometry behind the
// camera. More sophisticated methods will be used in the user-space
// functions
bool isBehind(glm::mat4 viewMatrix, glm::vec3 point, float limit = 0) {

    // Inverse view matrix for world space camera orientation
    auto inv = glm::inverse(viewMatrix);

    // Camera forward vector
    auto camNorm = glm::vec3(inv[2]);
    // Camera world space position
    auto camPos  = glm::vec3(inv[3]);

    // Invert the vector for correct orientation
    camNorm*=-1;

    // Dot product of point direction relative to camera
    auto dot = glm::dot(glm::normalize(point-camPos),camNorm);

    return dot < limit;
}


void UIManager::drawWorldSpaceLine(const glm::vec3& pos1, const glm::vec3& pos2,
                                   const MVP& mvp) {
    auto pvm = mvp.p * mvp.v * mvp.m;

    if (isBehind(mvp.v,pos1, VIEW_LIMIT) ||
        isBehind(mvp.v,pos2, VIEW_LIMIT)) {
        return;
    }

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = worldToScreen(pvm, pos1);
    glm::vec2 screenPos2 = worldToScreen(pvm, pos2);
    ImVec2 p1(screenPos1.x, screenPos1.y);
    ImVec2 p2(screenPos2.x, screenPos2.y);
    listBg->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 5);
}


// Draw a vertex by world space coords
// Debug function, not pretty, not fast
void UIManager::drawWorldSpaceVert(const glm::vec3& pos1,
                                   const glm::vec3& pos2,
                                   const glm::vec3& pos3,
                                   const MVP& mvp) {
    auto pvm = mvp.p * mvp.v * mvp.m;

    if (isBehind(mvp.v,pos1, VIEW_LIMIT) ||
        isBehind(mvp.v,pos2, VIEW_LIMIT) ||
        isBehind(mvp.v,pos3, VIEW_LIMIT)) {
        return;
    }

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = worldToScreen(pvm, pos1);
    glm::vec2 screenPos2 = worldToScreen(pvm, pos2);
    glm::vec2 screenPos3 = worldToScreen(pvm, pos3);
    ImVec2 p1(screenPos1.x, screenPos1.y);
    ImVec2 p2(screenPos2.x, screenPos2.y);
    ImVec2 p3(screenPos3.x, screenPos3.y);
    listBg->AddTriangle(p1, p2, p3, IM_COL32(255, 255, 255, 255), 5);
}


void UIManager::drawWorldSpaceCircle(const glm::vec3& pos,
                                     const MVP& mvp) {
    auto pvm = mvp.p * mvp.v * mvp.m;

    if (isBehind(mvp.v,pos, VIEW_LIMIT)) {
        return;
    }

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();

    glm::vec2 screenPos = worldToScreen(pvm, pos);
    ImVec2 center(screenPos.x, screenPos.y);
    auto color = IM_COL32(100, 155,255, 255);
    listBg->AddCircleFilled(center, 5, color);
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


void UIManager::drawMenuBar() {


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

void UIManager::vec3Handler(glm::vec3& vec, float dnLim, float upLim) {
            ImGui::SliderFloat("X", &vec.x,dnLim,upLim);
            ImGui::SliderFloat("Y", &vec.y,dnLim,upLim);
            ImGui::SliderFloat("Z", &vec.z,dnLim,upLim);
}

void parseNode(const ale::Model& model, int id) {

        ImGuiTreeNodeFlags base_flags =
                                    ImGuiTreeNodeFlags_OpenOnArrow |
                                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                    ImGuiTreeNodeFlags_SpanAvailWidth;
        auto n = model.nodes[id];
        ImGuiTreeNodeFlags node_flags = base_flags;

        std::string s = n.name.data();
        s.append(" idx: ");
        s.append(std::to_string(n.id));

        if (ImGui::TreeNodeEx((void*)(intptr_t)n.id, node_flags, s.data(),n.id)) {
            if (n.children.size() > 0) {
                for (auto c  : n.children) {
                    parseNode(model, c);
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(201,62,62,255));
                ImGui::Text("Leaf node");
                ImGui::PopStyleColor();
            }
            ImGui::TreePop();
        }
}


void UIManager::drawHierarchyUI(const ale::Model& model) {

    for (auto nID : model.rootNodes) {
        parseNode(model, nID);
    }
}

