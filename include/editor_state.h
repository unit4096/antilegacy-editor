#ifndef ALE_EDITOR_STATE
#define ALE_EDITOR_STATE

/*
    This header provides some enums to handle editor's state
*/
#pragma once

// int
#include <primitives.h>


namespace ale {

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

    GEditorState(){
        editorMode = OBJECT_MODE;
        transformMode = TRANSLATE_MODE;
        spaceMode = GLOBAL_MODE;
    };
};

} //namespace ale

#endif //ALE_EDITOR_STATE
