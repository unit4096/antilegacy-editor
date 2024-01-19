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
    float speed, sensitivity;
};


// FIXME: this class needs heavy refactoring. Make yaw and pitch functions that
// return and set camera orientation. No reason to keep two data structures 
// for the same thing
class Camera {
private:
    CameraData _data;
public:
    CameraMode mode;
    
    glm::vec3 targetPos;
    Camera();
    ~Camera();
    void toggleMode();
    CameraData getData();
    v2f getYawPitch();
    void setYawPitch(float yaw, float pitch);
    void setData(CameraData data);
    float getSpeed();
    void setSpeed(float speed);
    float getSensitivity();
    void setSensitivity(float sensitivity);
    glm::vec3 getForwardOrientation();
    void setForwardOrientation(float yaw, float pitch);
    void setForwardOrientation(glm::vec3 front);
    void setPosition(glm::vec3 newPos);
    void moveForwardLocal(const float speed);
    void moveBackwardLocal(const float speed);
    void moveLeftLocal(const float speed);
    void moveRightLocal(const float speed);
    void movePosGlobal(const glm::vec3 movementVector, const float speed);
}; // class Camera

} // namespace ale

#endif //ALE_CAMERA