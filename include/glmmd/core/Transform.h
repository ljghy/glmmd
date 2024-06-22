#ifndef GLMMD_CORE_TRANSFORM_H_
#define GLMMD_CORE_TRANSFORM_H_

#define GLM_FORCE_RADIANS
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
        return {translation * t,
                glm::slerp(glm::identity<glm::quat>(), rotation, t)};
    }

    friend Transform operator*(const float t, const Transform &transform)
    {
        return transform * t;
    }

    Transform operator*(const Transform &other) const
    {
        return {other.rotation * translation + other.translation,
                other.rotation * rotation};
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

    glm::mat4 toMatrix() const
    {
        return glm::translate(glm::mat4(1.f), translation) *
               glm::mat4_cast(rotation);
    }

    Transform inverse() const
    {
        glm::quat invRot = glm::inverse(rotation);
        return {invRot * -translation, invRot};
    }
};

inline constexpr Transform identityTransform{glm::vec3(0.f),
                                             glm::identity<glm::quat>()};

} // namespace glmmd

#endif
