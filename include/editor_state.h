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


struct GEditorState {
    GEditorMode editorMode;
    GTransformMode transformMode;
    GSpaceMode spaceMode;

    sp<ale::geo::REMesh> currentREMesh;
    sp<ale::ViewMesh>    currentViewMesh;
    sp<ale::Model>       currentModel;
    sp<ale::Node>        currentModelNode;

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
