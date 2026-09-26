#ifndef GLMMD_CORE_TRANSFORM_H_
#define GLMMD_CORE_TRANSFORM_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glmmd {

// A rigid transform (rotation + translation), applied as `rotate then
// translate`. Composition uses `operator*`: `a * b` yields the transform that
// applies `a` first, then `b` (i.e. b is the parent). Scaling by a float slerps
// the rotation from identity and scales the translation linearly, which is used
// for weighted blending, not for a similarity transform.
struct Transform {
  glm::vec3 translation;
  glm::quat rotation;

  Transform operator*(const float t) const {
    return {.translation = translation * t,
            .rotation = glm::slerp(glm::identity<glm::quat>(), rotation, t)};
  }

  friend Transform operator*(const float t, const Transform &transform) {
    return transform * t;
  }

  Transform operator*(const Transform &other) const {
    return {.translation = other.rotation * translation + other.translation,
            .rotation = other.rotation * rotation};
  }

  glm::vec3 operator*(const glm::vec3 &v) const {
    return rotation * v + translation;
  }

  Transform &operator*=(const Transform &other) {
    *this = *this * other;
    return *this;
  }

  Transform &operator*=(const float t) {
    *this = *this * t;
    return *this;
  }

  Transform inverse() const {
    glm::quat invRot = glm::inverse(rotation);
    return {.translation = invRot * -translation, .rotation = invRot};
  }

  static const Transform identity;
};

inline const Transform Transform::identity{
    .translation = glm::vec3(0.f), .rotation = glm::identity<glm::quat>()};

} // namespace glmmd

#endif
