#ifndef GLMMD_VIEWER_CAMERA_H_
#define GLMMD_VIEWER_CAMERA_H_

#include <glm/glm.hpp>

namespace glmmd
{

enum class CameraProjectionType
{
    Perspective,
    Orthographic
};

struct Camera
{
public:
    Camera();

    glm::mat4 view() const;
    glm::mat4 proj() const;

    void rotate(float dy, float dp);
    void rotateAround(const glm::vec3 &point, float dy, float dp);

    void resize(int width, int height);

private:
    void update();

public:
    CameraProjectionType projType = CameraProjectionType::Perspective;

    glm::vec3 worldUp  = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    float fovy  = glm::radians(45.f);
    float near  = 0.1f;
    float far   = 1000.f;
    float width = 40.f;

    float yaw   = glm::radians(90.f);
    float pitch = glm::radians(-10.f);

    int   viewportWidth  = 1;
    int   viewportHeight = 1;
    float aspect         = 1.f;
};

} // namespace glmmd

#endif
