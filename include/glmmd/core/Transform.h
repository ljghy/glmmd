#ifndef GLMMD_CORE_TRANSFORM_H_
#define GLMMD_CORE_TRANSFORM_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glmmd
{

struct Transform
{
    glm::vec3 translation;
    glm::quat rotation;

    Transform operator*(const float t) const
    {
        return {.translation = translation * t,
                .rotation =
                    glm::slerp(glm::identity<glm::quat>(), rotation, t)};
    }

    friend Transform operator*(const float t, const Transform &transform)
    {
        return transform * t;
    }

    Transform operator*(const Transform &other) const
    {
        return {.translation = other.rotation * translation + other.translation,
                .rotation    = other.rotation * rotation};
    }

    glm::vec3 operator*(const glm::vec3 &v) const
    {
        return rotation * v + translation;
    }

    Transform &operator*=(const Transform &other)
    {
        *this = *this * other;
        return *this;
    }

    Transform &operator*=(const float t)
    {
        *this = *this * t;
        return *this;
    }

    Transform inverse() const
    {
        glm::quat invRot = glm::inverse(rotation);
        return {.translation = invRot * -translation, .rotation = invRot};
    }

    static const Transform identity;
};

inline const Transform Transform::identity{
    .translation = glm::vec3(0.f), .rotation = glm::identity<glm::quat>()};

} // namespace glmmd

#endif
