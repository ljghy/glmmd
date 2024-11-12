#ifndef GLMMD_CORE_CAMERA_H_
#define GLMMD_CORE_CAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glmmd
{

struct Camera
{
    enum ProjType
    {
        Perspective,
        Orthographic
    };

    Camera();

    const glm::vec3 &front() const;
    const glm::vec3 &up() const;
    const glm::vec3 &right() const;

    const glm::mat4 &view() const;
    const glm::mat4 &proj() const;

    void setRotation(const glm::vec3 &eulerAngles);

    void rotate(float dy, float dp);

    void resize(int width, int height);

    void update();

    ProjType projType = Perspective;

    glm::vec3 target;
    glm::quat rotation;

    float distance;
    float fov;
    float nearZ;
    float farZ;
    float width;

    int viewportWidth;
    int viewportHeight;

private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;

    glm::mat4 m_view;
    glm::mat4 m_proj;

    float m_aspect;
};

} // namespace glmmd

#endif
