#include <glm/gtc/matrix_transform.hpp>

#include <glmmd/core/Camera.h>

namespace glmmd
{

Camera::Camera() { update(); }

void Camera::resize(int w, int h)
{
    viewportWidth  = w;
    viewportHeight = h;
    aspect         = static_cast<float>(w) / h;
}

glm::mat4 Camera::view() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::proj() const
{
    if (projType == CameraProjectionType::Perspective)
    {
        return glm::perspective(fovy, aspect, nearZ, farZ);
    }
    else
    {
        float height = width / aspect;
        return glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f,
                          height * 0.5f, nearZ, farZ);
    }
}

void Camera::rotate(float dy, float dp)
{
    constexpr float limit = glm::radians(89.f);

    yaw += dy;
    pitch += dp;
    pitch = glm::clamp(pitch, -limit, limit);
    update();
}

void Camera::rotateAround(const glm::vec3 &point, float dy, float dp)
{
    constexpr float limit = glm::radians(89.f);
    if (glm::abs(pitch + dp) < limit)
    {
        position = glm::rotate(glm::mat4(1.f), dy, worldUp) *
                       glm::rotate(glm::mat4(1.f), dp, right) *
                       glm::vec4(position - point, 1.f) +
                   glm::vec4(point, 0.f);

        rotate(-dy, dp);
    }
}

void Camera::update()
{
    float cp = glm::cos(pitch);

    front = glm::normalize(glm::vec3(cos(yaw) * cp, sin(pitch), sin(yaw) * cp));
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}

} // namespace glmmd