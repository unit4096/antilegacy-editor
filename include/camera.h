#ifndef ALE_CAMERA
#define ALE_CAMERA


// ext
#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

// int
#include <primitives.h>


namespace ale {

// ale::Camera viewing mode
enum class CameraMode {
    FREE,
    ARCBALL
};

// Struct for the ale::Camera class
struct CameraData {    
    glm::vec3 position;
    glm::vec3 front, up;
    glm::vec3 worldUp;
    float fov;
    float yaw, pitch;
    float nearPlane, farPlane;
};


// FIXME: this class needs heavy refactoring. Make yaw and pitch functions that
// return and set camera orientation. No reason to keep two data structures 
// for the same thing
class Camera {
private:
    CameraData data;
public:
    CameraMode mode;
    
    glm::vec3 targetPos;
    Camera();
    ~Camera();
    void toggleMode();
    CameraData getData();
    v2f getYawPitch();
    void setYawPitch(float yaw, float pitch);
    void setData(CameraData _data);
    glm::vec3 getForwardOrientation();
    void setForwardOrientation(float yaw, float pitch);
    void setForwardOrientation(glm::vec3 _front);
    void moveForwardLocal(const float speed);
    void moveBackwardLocal(const float speed);
    void moveLeftLocal(const float speed);
    void moveRightLocal(const float speed);
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

// Toggle viewing mode. Now toggless between free camera and arcball camera
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

// Get camera data struct
CameraData Camera::getData() {
    return data;
}

// Set camera data struct
void Camera::setData(CameraData _data) {
    data = _data;
}

// Get yaw and pitch in degrees. X for yaw, Y for pitch
v2f Camera::getYawPitch() {
    return v2f(this->data.yaw,this->data.pitch);
}

// Set the orientation vector using yaw and pitch variables in degrees
void Camera::setYawPitch(float _yaw, float _pitch) {
    this->data.yaw = _yaw;
    this->data.pitch = _pitch;
    this->setForwardOrientation(_yaw, _pitch);
}

// Get the orientation (forward) vector 
glm::vec3 Camera::getForwardOrientation() {
    return this->data.front;
}

// Sets the orientation vector of the camera by yaw and pitch (in degrees)
void Camera::setForwardOrientation(float yaw, float pitch) {

    float cosYawRad = std::cos(glm::radians(yaw));
    float sinYawRad = std::sin(glm::radians(yaw));
    float cosPitchRad = std::cos(glm::radians(pitch));
    float sinPitchRad = std::sin(glm::radians(pitch));

    glm::vec3 forwardVector;
    forwardVector.x = cosPitchRad * sinYawRad;
    forwardVector.y = sinPitchRad;
    forwardVector.z = cosPitchRad * cosYawRad;

    this->data.front = glm::normalize(forwardVector);
}

// Sets the orientation vector of the camera by a 3D vector
void Camera::setForwardOrientation(glm::vec3 _front) {
    this->data.front = _front;
}


// Moves the camera in the direction of the forward orientation vector
void Camera::moveForwardLocal(const float speed) {
    glm::vec3 _forward = this->data.front;
    _forward.x = -_forward.x;
    this->data.position +=  _forward * -speed;
}

// Moves the camera in the direction opposite to the forward orientation vector
void Camera::moveBackwardLocal(const float speed) {
    moveForwardLocal(-speed);
}

// Moves the camera left from the forward direction
void Camera::moveLeftLocal(const float speed) {
    glm::vec3 _forward = this->data.front;
    _forward.x = -_forward.x;
    this->data.position += speed * glm::normalize(glm::cross(_forward, this->data.up));
}

// Moves the camera right from the forward direction
void Camera::moveRightLocal(const float speed) {
    glm::vec3 _forward = this->data.front;
    _forward.x = -_forward.x;
    this->data.position += speed * -glm::normalize(glm::cross(_forward, this->data.up));
}

// Moves the camera in the direction of the specified vector
void Camera::movePosGlobal(const glm::vec3 movementVector, const float speed) {
    this->data.position = this->data.position + (movementVector * speed);
}

} // namespace ale

#endif //ALE_CAMERA