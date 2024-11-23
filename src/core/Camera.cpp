#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <glmmd/core/Camera.h>

namespace glmmd
{

Camera::Camera() { update(); }

void Camera::resize(int w, int h)
{
    viewportWidth  = w;
    viewportHeight = h;
    m_aspect       = static_cast<float>(w) / h;
}

void Camera::setRotation(const glm::vec3 &eulerAngles)
{
    rotation = glm::quat_cast(
        glm::eulerAngleYZX(eulerAngles.y, eulerAngles.z, -eulerAngles.x));
}

const glm::vec3 &Camera::front() const { return m_front; }
const glm::vec3 &Camera::up() const { return m_up; }
const glm::vec3 &Camera::right() const { return m_right; }

const glm::mat4 &Camera::view() const { return m_view; }
const glm::mat4 &Camera::proj() const { return m_proj; }

void Camera::rotate(float dy, float dp)
{
    glm::quat qy = glm::angleAxis(dy, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qp = glm::angleAxis(dp, glm::vec3(-1.0f, 0.0f, 0.0f));
    rotation     = glm::normalize(qy * rotation * qp);
}

void Camera::update()
{
    glm::mat3 axes = glm::mat3_cast(rotation);

    m_front    = axes[2];
    m_up       = axes[1];
    m_right    = -axes[0];
    m_position = target - m_front * distance;

    m_view = glm::lookAt(m_position, target, up());
    if (projType == Perspective)
    {
        m_proj = glm::perspective(fov, m_aspect, zNear, zFar);
    }
    else
    {
        float height = width / m_aspect;

        m_proj = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f,
                            height * 0.5f, zNear, zFar);
    }
}

void Camera::getFrustumCorners(glm::vec3 *corners, float zNear_,
                               float zFar_) const
{
    glm::mat4 invVP = glm::inverse(m_proj * m_view);

    glm::vec4 pz = m_proj * glm::vec4(0.f, 0.f, -zNear_, 1.f);
    zNear_       = pz.z / pz.w;
    pz           = m_proj * glm::vec4(0.f, 0.f, -zFar_, 1.f);
    zFar_        = pz.z / pz.w;

    glm::vec4 ndcPoints[]{{1.f, 1.f, zNear_, 1.f},   {1.f, -1.f, zNear_, 1.f},
                          {-1.f, -1.f, zNear_, 1.f}, {-1.f, 1.f, zNear_, 1.f},
                          {1.f, 1.f, zFar_, 1.f},    {1.f, -1.f, zFar_, 1.f},
                          {-1.f, -1.f, zFar_, 1.f},  {-1.f, 1.f, zFar_, 1.f}};

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 p = invVP * ndcPoints[i];
        corners[i]  = glm::vec3(p) / p.w;
    }
}

} // namespace glmmd