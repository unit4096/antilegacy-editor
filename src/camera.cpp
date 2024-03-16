#include <camera.h>

Camera::Camera() {

    Transform _newTransform {
        .pos = glm::vec3(0),
        .rot = glm::mat4(1),
        // Does not matter for the camera
        .sca = glm::mat4(1),
    };

    CameraData _newData {
        .transform = _newTransform,
        .fov = 45.0f,
        .yaw = 0,
        .pitch = 0,
        .nearPlane = 0.001f,
        .farPlane = 10000.0f,
    };

    _data = _newData;
    mode = CameraMode::ARCBALL;
    _targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
}

Camera::~Camera() {}

void Camera::setDelta(float delta) {
    _delta = delta;
}

// Toggle viewing mode. Now toggless between FREE and ARCBALL
void Camera::toggleMode() {
    switch (mode) {
    case CameraMode::FREE:
        mode = CameraMode::ARCBALL;
        break;

    case CameraMode::ARCBALL:
        mode = CameraMode::FREE;
        break;

    }
}

// GENERIC DATA

// Get camera data struct
CameraData Camera::getData() {
    return _data;
}

// Set camera data struct
void Camera::setData(CameraData data) {
    _data = data;
}

// VIEWPORT DATA

float Camera::getFov() {
    return _data.fov;
}

void Camera::setFov(float fov) {
    _data.fov = fov;
}

float Camera::getPlaneFar() {
    return _data.farPlane;
}


void Camera::setPlaneFar(float plane) {
    _data.farPlane = plane;
}

float Camera::getPlaneNear() {
    return _data.nearPlane;
}

void Camera::setPlaneNear(float plane) {
    _data.nearPlane = plane;
}

// CAMERA MOVEMENT VARIABLES

float Camera::getSpeed() {
    return _data.speed;
}

void Camera::setSpeed(float speed) {
    _data.speed = speed;
}

float Camera::getSensitivity() {
    return _data.sensitivity;
}

void Camera::setSensitivity(float sensitivity) {
    _data.sensitivity = sensitivity;
}

// CAMERA ROTATION

// Get yaw and pitch in degrees. X for yaw, Y for pitch
v2f Camera::getYawPitch() {
    return v2f(_data.yaw,_data.pitch);
}

// Set the orientation quat using yaw and pitch variables in degrees
void Camera::setOrientation(float yaw, float pitch) {
    _data.yaw = yaw;
    _data.pitch = pitch;
    glm::mat4x4 mat(1.0f);

    // Convert to radians
    float yawAng = glm::radians(yaw);
    float pitchAng = glm::radians(pitch);

    // Pitch vec is not always parallel to any axis
    auto pitchVec = glm::vec3(glm::cos(yawAng), 0.0f, glm::sin(yawAng));

    // Yaw, then pitch, then translation
    mat = glm::rotate(mat, yawAng, GLOBAL_UP);
    mat = glm::rotate(mat, pitchAng, pitchVec);
    mat = glm::translate(mat, -_data.transform.pos);

    _setOrientationInternal(glm::quat_cast(mat));
}

// Get vec3 pointing in the camera direction
glm::vec3 Camera::getForwardVec() {
    return _data.front;
}

// Get camera orientation quat in global space
glm::quat Camera::getOrientation() {
    return _data.transform.rot;
}

// Some internal orientation functions

void Camera::setOrientation(glm::mat4 rotation) {
    _setOrientationInternal(rotation);
}

void Camera::setOrientation(glm::quat rotation) {
    _setOrientationInternal(glm::mat4_cast(rotation));
}

void Camera::_setOrientationInternal(glm::quat newRotation) {
    auto rot = glm::normalize(newRotation);
    _data.transform.rot = rot;
    _setDirVecInternal(rot);
}

void Camera::_setYawPitchInternal(glm::quat rotation) {
    glm::vec3 angles = glm::eulerAngles(rotation);
    _data.yaw = angles.x;
    _data.pitch = angles.y;
}

void Camera::_setDirVecInternal(glm::quat rotation) {

    auto invRot = glm::inverse(rotation);
    auto forward = glm::normalize(glm::vec3(invRot * GLOBAL_FORWARD));
    auto up = glm::normalize(glm::vec3(invRot * GLOBAL_UP));

    _data.front = forward;
    _data.up = up;
}

// CAMERA POSITION

glm::vec3 Camera::getPos() {
    return _data.transform.pos;
}


void Camera::setPos(glm::vec3 newPos) {
    _data.transform.pos = newPos;
}

//================================
// MOVEMENT CONTROLS

// Moves the camera in the direction of the forward orientation vector
void Camera::moveForwardLocal() {
    _data.transform.pos +=  _data.front * _data.speed * (_delta);
}

// Moves the camera in the direction opposite to the forward orientation vector
void Camera::moveBackwardLocal() {
    _data.transform.pos +=  _data.front * -_data.speed * (_delta);
}

// Moves the camera left from the forward direction
void Camera::moveLeftLocal() {
    _data.transform.pos += _data.speed * (_delta) * -glm::normalize(glm::cross(_data.front, _data.up));
}

// Moves the camera right from the forward direction
void Camera::moveRightLocal() {
    _data.transform.pos += _data.speed * (_delta) * glm::normalize(glm::cross(_data.front, _data.up));
}

// Moves the camera in the direction of the specified vector
void Camera::movePosGlobal(const glm::vec3 movementVector) {
    _data.transform.pos = _data.transform.pos + (movementVector * _data.speed * (_delta));
}

// Set target for ARCBALL mode
void Camera::setTarget(glm::vec3 target) {
    _targetPos = target;
}

// Get target for ARCBALL mode
glm::vec3 Camera::getTarget() {
    return _targetPos;
}

//================================
