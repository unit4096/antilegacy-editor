#ifndef ALE_EDITOR_STATE
#define ALE_EDITOR_STATE

/*
    This header provides some enums to handle editor's state
*/
#pragma once
// ext
#include <type_traits>

// int
#include <primitives.h>
#include <re_mesh.h>

namespace ale {


template<typename T>
concept Enum = std::is_enum_v<T>;


const std::vector<std::string> GEditorMode_Names { "OBJECT_MODE", "MESH_MODE", "UV_MODE", "EDITOR_MODE_MAX", };
const std::vector<std::string> GTransformMode_Names { "TRANSLATE_MODE", "ROTATE_MODE", "SCALE_MODE", "COMBINED_MODE", "TRANSFORM_MODE_MAX", };
const std::vector<std::string> GSpaceMode_Names { "LOCAL_MODE", "GLOBAL_MODE", "SPACE_MODE_MAX", };
const std::vector<std::string> UI_DRAW_TYPE_Names { "CIRCLE", "LINE", "VERT", "UI_DRAW_TYPE_MAX", };


enum GEditorMode {
    OBJECT_MODE,
    MESH_MODE,
    UV_MODE,
    EDITOR_MODE_MAX,
};


enum GTransformMode {
    TRANSLATE_MODE,
    ROTATE_MODE,
    SCALE_MODE,
    COMBINED_MODE,
    TRANSFORM_MODE_MAX,
};


enum GSpaceMode {
    LOCAL_MODE,
    GLOBAL_MODE,
    SPACE_MODE_MAX,
};

enum UI_DRAW_TYPE {
    CIRCLE,
    LINE,
    VERT,
    AABB,
    UI_DRAW_TYPE_MAX,
};


struct GEditorState {
    GEditorMode editorMode;
    GTransformMode transformMode;
    GSpaceMode spaceMode;

    ale::geo::REMesh*   currentREMesh;
    ale::ViewMesh*   currentViewMesh;
    ale::Model*      currentModel;

    ale::Node*          currentModelNode;

    // TODO: Find a more powerful way to store this data

    std::vector<ale::geo::Vert*> selectedVerts;
    std::vector<ale::geo::Edge*> selectedEdges;
    std::vector<ale::geo::Face*> selectedFaces;

    std::vector<std::pair<std::vector<glm::vec3>, UI_DRAW_TYPE>> uiDrawQueue;


    GEditorState(){
        editorMode = OBJECT_MODE;
        transformMode = TRANSLATE_MODE;
        spaceMode = GLOBAL_MODE;
    }

    void setNextModeEditor() {
        _enumCycle(editorMode, EDITOR_MODE_MAX);
    }

    void setNextModeTransform() {
        _enumCycle(transformMode, TRANSFORM_MODE_MAX);
    }

    void setNextModeSpace() {
        _enumCycle(spaceMode, SPACE_MODE_MAX);
    }

private:
    // Recieves a reference to enum and its max value.
    // Loops over the enum
    template<Enum T>
    void _enumCycle(T& current, T max) {
        current = static_cast<T>((current + 1) % max);
    }
};

} //namespace ale

#endif //ALE_EDITOR_STATE
