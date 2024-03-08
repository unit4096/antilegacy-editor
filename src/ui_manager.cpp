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

    auto& io = ImGui::GetIO();
    auto size = glm::vec2(io.DisplaySize.x,io.DisplaySize.y);

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = ale::geo::worldToScreen(pvm, size,  pos1);
    glm::vec2 screenPos2 = ale::geo::worldToScreen(pvm, size,  pos2);
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
    auto& io = ImGui::GetIO();
    auto size = glm::vec2(io.DisplaySize.x,io.DisplaySize.y);

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();
    glm::vec2 screenPos1 = ale::geo::worldToScreen(pvm, size, pos1);
    glm::vec2 screenPos2 = ale::geo::worldToScreen(pvm, size, pos2);
    glm::vec2 screenPos3 = ale::geo::worldToScreen(pvm, size, pos3);
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
    auto& io = ImGui::GetIO();
    auto size = glm::vec2(io.DisplaySize.x,io.DisplaySize.y);

    ImDrawList* listBg = ImGui::GetBackgroundDrawList();

    glm::vec2 screenPos = ale::geo::worldToScreen(pvm,size, pos);
    ImVec2 center(screenPos.x, screenPos.y);
    auto color = IM_COL32(100, 155,255, 255);
    listBg->AddCircleFilled(center, 5, color);
}


[[maybe_unused]]
static void drawImGuiGrid(){
    ale::Tracer::log("Not implemented!", ale::Tracer::ERROR);
}


// Draws ImGuizmo gizmo that takes mvp, editor state, and a transform
// FIXME: How do i use this to manipulate sets of primitives?
void UIManager::drawImGuiGizmo(glm::mat4& view, glm::mat4& proj, glm::mat4& model, GEditorState& state){
    UIManager::flipProjection(proj);
    float* _view = geo::glmMatToPtr(view);
    float* _proj = geo::glmMatToPtr(proj);
    float* _model = geo::glmMatToPtr(model);

    auto operation = ImGuizmo::OPERATION::TRANSLATE;
    auto space = ImGuizmo::MODE::WORLD;

    switch (state.transformMode) {
        case TRANSLATE_MODE:
            operation = ImGuizmo::OPERATION::TRANSLATE;
            break;
        case ROTATE_MODE:
            operation = ImGuizmo::OPERATION::ROTATE;
            break;
        case SCALE_MODE:
            operation = ImGuizmo::OPERATION::SCALE;
            break;
        case COMBINED_MODE:
            operation = ImGuizmo::OPERATION::UNIVERSAL;
            break;
        case TRANSFORM_MODE_MAX:
            trc::log("This enum is just the enum's size! Do not use it as a valid value!",trc::ERROR);
            break;
    }

    ImGuizmo::Manipulate(_view, _proj, operation, space, _model);
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
// Handles a glm vec3 as X Y Z sliders
void UIManager::vec3Handler(glm::vec3& vec, float dnLim, float upLim) {
            ImGui::SliderFloat("X", &vec.x,dnLim,upLim);
            ImGui::SliderFloat("Y", &vec.y,dnLim,upLim);
            ImGui::SliderFloat("Z", &vec.z,dnLim,upLim);
}

// Handles an arbitary std::vector of floats
// TODO: Maybe find a nice way to have names and arbitrary size vectors
void UIManager::vec3Handler(std::vector<float>& vec, float dnLim, float upLim) {
    // I know I know, but...
    assert(vec.size() == 3);
    ImGui::SliderFloat("X", &vec[0], dnLim, upLim);
    ImGui::SliderFloat("Y", &vec[1], dnLim, upLim);
    ImGui::SliderFloat("Z", &vec[2], dnLim, upLim);
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


// Draws each node in the scene as a circle. Takes the model and a
// flipped mvp matrix
void UIManager::drawNodeRoots(const ale::Model& model, const MVP& pvm) {
    for (auto& n : model.nodes) {
        auto t = n.transform;
        auto pos = glm::vec3(t[3][0], t[3][1], t[3][2]);
        ale::UIManager::drawWorldSpaceCircle(pos, pvm);
    }
}

// Draws each node in the scene as a circle. Takes
// a pair of <points, type> and a flipped mvp matrix
void UIManager::drawVectorOfPrimitives(const std::vector<glm::vec3>& vec, UI_DRAW_TYPE mode, const MVP& pvm) {
    if (mode == UI_DRAW_TYPE::CIRCLE && (vec.size() == 1)) {
            ale::UIManager::drawWorldSpaceCircle(vec[0], pvm);
    }

    // Not enough points to draw a line
    if (vec.size() < 2) {
        return;
    }

    // Connect all points in one line
    if (mode == UI_DRAW_TYPE::LINE) {
        for (int i = 1; i < vec.size(); i++) {
            ale::UIManager::drawWorldSpaceLine(vec[i-1], vec[i], pvm);
        }
    }

    // Vertices need 3 and only 3 points
    if (mode == UI_DRAW_TYPE::VERT && (vec.size() % 3 == 0)) {
        for (int i = 0; i < vec.size(); i+=3) {
            ale::UIManager::drawWorldSpaceVert(vec[i+0],
                                               vec[i+1],
                                               vec[i+2], pvm);
        }
    }
}



// Basic template for a camera control widget. Takes a shared_ptr to
// ale::Camera
void UIManager::CameraControlWidgetUI(sp<ale::Camera> cam) {

    CameraData camData = cam->getData();
    // CAMERA CONTROL START
    ale::UIManager::vec3Handler(camData.transform.pos,-100.0f, 100.0f);

    ImGui::SliderFloat("FOV", &camData.fov, 10.0f, 90.0f);
    ImGui::SliderFloat("YAW", &camData.yaw, 0.0f, 360.0f);
    ImGui::SliderFloat("PITCH", &camData.pitch, -90.0f, 90.0f);
    ImGui::SliderFloat("SPEED", &camData.speed, 0.0001f, 10.0f);

    if (ImGui::Button("Toggle Camera mode"))
        cam->toggleMode();

    std::string mode_name = cam->mode==CameraMode::ARCBALL
                            ?"ARCBALL"
                            :"FREE";
    ImGui::SameLine();
    ImGui::Text("%s",mode_name.data());
    cam->setData(camData);
    /// CAMERA CONTROL END
}


