#ifndef ALE_CAMERA
#define ALE_CAMERA


// ext
#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#endif // GLM

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// int
#include <primitives.h>


namespace ale {

// ale::Camera viewing mode
enum class CameraMode {
    FREE,
    ARCBALL
};

// ale::Camera direction
enum class CameraDirection {
    FORWARD,
    BACK,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

// Struct for the ale::Camera class
struct CameraData {
    Transform transform;
    glm::vec3 front, up;
    glm::vec3 worldUp;
    float fov;
    float yaw, pitch;
    float nearPlane, farPlane;
    float speed, sensitivity;
};

class Camera {
private:
    CameraData _data;
    const glm::vec3 GLOBAL_UP = glm::vec3(0,1,0);
    const glm::vec3 GLOBAL_FORWARD = glm::vec3(0,0,-1);
    const glm::vec3 GLOBAL_RIGHT = glm::vec3(1,0,0);
    void _setOrientationInternal(glm::quat newRotation);
    void _setDirVecInternal(glm::quat rotation);
    void _setYawPitchInternal(glm::quat rotation);
    glm::vec3 _targetPos;
public:
    CameraMode mode;
    Camera();
    ~Camera();

    // Generic data set (use with utmost care)

    void setData(CameraData data);
    CameraData getData();

    // View data

    float getFov();
    void setFov(float fov);
    float getPlaneFar();
    void setPlaneFar(float plane);
    float getPlaneNear();
    void setPlaneNear(float plane);

    // ARCBALL or FREE modes
    void toggleMode();

    // Camera orientation (rotation) in global space

    void setOrientation(float yaw, float pitch);
    void setOrientation(glm::quat orientation);
    void setOrientation(glm::mat4 rotatioh);
    // FIXME: custom data types of this sort are no good
    v2f getYawPitch();
    glm::quat getOrientation();
    // Normalized vector with the direction of the camera
    glm::vec3 getForwardVec();

    // Camera position

    void setPos(glm::vec3 newPos);
    glm::vec3 getPos();

    // Camera control variables

    float getSpeed();
    void setSpeed(float speed);
    float getSensitivity();
    void setSensitivity(float sensitivity);

    // Movement commands

    void moveForwardLocal(const float speed);
    void moveBackwardLocal(const float speed);
    void moveLeftLocal(const float speed);
    void moveRightLocal(const float speed);
    void movePosGlobal(const glm::vec3 movementVector, const float speed);

    void setTarget(glm::vec3 target);
    glm::vec3 getTarget();

}; // class Camera

} // namespace ale

#endif //ALE_CAMERA
