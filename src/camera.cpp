#include <camera.h>

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
    this->_data = _data;

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
    return _data;
}

// Set camera data struct
void Camera::setData(CameraData data) {
    this->_data = data;
}

float Camera::getSpeed() {
    return this->_data.speed;
}

void Camera::setSpeed(float speed) {
    this->_data.speed = speed;
}

float Camera::getSensitivity() {
    return this->_data.sensitivity;
}

void Camera::setSensitivity(float sensitivity) {
    this->_data.sensitivity = sensitivity;
}

// Get yaw and pitch in degrees. X for yaw, Y for pitch
v2f Camera::getYawPitch() {
    return v2f(this->_data.yaw,this->_data.pitch);
}

// Set the orientation vector using yaw and pitch variables in degrees
void Camera::setYawPitch(float yaw, float pitch) {
    this->_data.yaw = yaw;
    this->_data.pitch = pitch;
    this->setForwardOrientation(yaw, pitch);
}

// Get the orientation (forward) vector 
glm::vec3 Camera::getForwardOrientation() {
    return this->_data.front;
}

// FIXME: _data.front appears to be not normalized and possibly incorrect
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


    auto res = glm::normalize(forwardVector);

    this->_data.front = res;
}

// Sets the orientation vector of the camera by a 3D vector
void Camera::setForwardOrientation(glm::vec3 front) {
    this->_data.front = front;
}

void Camera::setPosition(glm::vec3 newPos) {
    this->_data.position = newPos;    
}

// Moves the camera in the direction of the forward orientation vector
void Camera::moveForwardLocal(const float speed) {
    glm::vec3 _forward = this->_data.front;
    _forward.x = -_forward.x;
    this->_data.position +=  _forward * -speed;
}

// Moves the camera in the direction opposite to the forward orientation vector
void Camera::moveBackwardLocal(const float speed) {
    moveForwardLocal(-speed);
}

// Moves the camera left from the forward direction
void Camera::moveLeftLocal(const float speed) {
    glm::vec3 _forward = this->_data.front;
    _forward.x = -_forward.x;
    this->_data.position += speed * glm::normalize(glm::cross(_forward, this->_data.up));
}

// Moves the camera right from the forward direction
void Camera::moveRightLocal(const float speed) {
    glm::vec3 _forward = this->_data.front;
    _forward.x = -_forward.x;
    this->_data.position += speed * -glm::normalize(glm::cross(_forward, this->_data.up));
}

// Moves the camera in the direction of the specified vector
void Camera::movePosGlobal(const glm::vec3 movementVector, const float speed) {
    this->_data.position = this->_data.position + (movementVector * speed);
}

glm::vec3 Camera::getPos() {
    return this->_data.position;
}
