#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

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
    CameraData data;
public:
    CameraMode mode;
    
    glm::vec3 targetPos;
    Camera();
    ~Camera();
    CameraData getData();
    void toggleMode();
    void setData(CameraData _data);
    void movePosLocal(const glm::vec3 movementVector, const float speed);
    void movePosGlobal(const glm::vec3 movementVector, const float speed);
}; // class Camera

Camera::Camera() {
    CameraData _data;
    _data.fov = 45.0f;
    _data.farPlane = 10000.0f;
    _data.nearPlane = 0.001f;
    _data.up = glm::vec3(0,1,0);
    _data.front = glm::vec3(1,0,0);
    _data.position = glm::vec3(0.0f,0.0f,1.0f);
    _data.pitch = 0;
    _data.yaw = 0;
    this->data = _data;

    mode = CameraMode::ARCBALL;
    targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
}

Camera::~Camera() {}

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

CameraData Camera::getData() {
    return data;
}

void Camera::setData(CameraData _data) {
    data = _data;
}

void Camera::movePosLocal(const glm::vec3 movementVector, const float speed) {
    std::cout << "ALE: Local movement not implemented! \n";    
}

void Camera::movePosGlobal(const glm::vec3 movementVector, const float speed) {
    this->data.position = this->data.position + (movementVector * speed);
}

} // namespace ale

#endif //ALE_CAMERA