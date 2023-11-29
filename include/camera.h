#pragma once
#include <glm/glm.hpp>

#ifndef ALE_CAMERA
#define ALE_CAMERA


namespace ale {

enum class CameraMode {
    FREE,
    ARCBALL
};

struct CameraData {    
    glm::vec3 position;
    glm::vec3 front, up;
    glm::vec3 worldUp;
    float fov;
    float yaw, pitch;
    float nearPlane, farPlane;
};

class Camera {
private:
public:
    CameraMode mode;
    CameraData data;
    glm::vec3 targetPos;
    Camera();
    ~Camera();
    CameraData getData();
    void toggleMode();
    void setData(CameraData _data);
}; // class Camera



Camera::Camera() {
    CameraData _data;
    _data.fov = 45.0f;
    _data.farPlane = 10000.0f;
    _data.nearPlane = 0.001f;
    _data.up = glm::vec3(0,0,1);
    _data.front = glm::vec3(1,0,0);
    _data.position = glm::vec3(2.0f,2.0f,3.0f);
    data = _data;

    mode = CameraMode::ARCBALL;
    targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
}

Camera::~Camera() {

}

void Camera::toggleMode() {
    switch (mode) {
    case CameraMode::FREE:
        mode = CameraMode::ARCBALL;
        break;

    case CameraMode::ARCBALL:
        mode = CameraMode::FREE;
        break;
    
    default:
        break;
    }
}


} // namespace ale

#endif //ALE_CAMERA