#pragma once

#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#endif // GLM

#include <primitives.h>
#include <vector>

#ifndef ALE_UI
#define ALE_UI

namespace ale {
namespace ui {


class UIManager {
public:
    static void DrawGrid();
};



void UIManager::DrawGrid() {
    return;
}

} // namespace ui
} // namespace ale

#endif // ALE_UI
