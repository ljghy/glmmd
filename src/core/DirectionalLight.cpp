#include <glmmd/core/DirectionalLight.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace glmmd {

const glm::mat4 &DirectionalLight::view() const { return m_view; }

const glm::mat4 &DirectionalLight::proj() const { return m_proj; }

static glm::mat4 lightOrientation(glm::vec3 direction) {
  direction = glm::dot(direction, direction) > 1e-8f
                  ? glm::normalize(direction)
                  : glm::vec3(0.f, -1.f, 0.f);
  const glm::vec3 up = std::abs(direction.y) > 0.99f ? glm::vec3(0.f, 0.f, 1.f)
                                                     : glm::vec3(0.f, 1.f, 0.f);
  return glm::lookAt(glm::vec3(0.f), direction, up);
}

constexpr float boundingBoxEnlargeFactor = 1.5f;

void DirectionalLight::update() {
  direction = glm::dot(direction, direction) > 1e-8f
                  ? glm::normalize(direction)
                  : glm::vec3(0.f, -1.f, 0.f);
  m_view =
      lightOrientation(direction) * glm::translate(glm::mat4(1.f), -position);
  m_proj = glm::ortho(-extents.x, extents.x, -extents.y, extents.y, -extents.z,
                      extents.z);
}

void DirectionalLight::updateFrustum(int count, const glm::vec3 *points) {
  if (count == 0)
    return;

  glm::mat4 view = lightOrientation(direction);

  glm::vec3 p = glm::vec3(view * glm::vec4(points[0], 1.f));
  glm::vec3 lb = p;
  glm::vec3 ub = p;

  for (size_t i = 1; i < count; ++i) {
    p = glm::vec3(view * glm::vec4(points[i], 1.f));
    lb = glm::min(lb, p);
    ub = glm::max(ub, p);
  }

  extents = (boundingBoxEnlargeFactor * 0.5f) * (ub - lb);
  extents.z *= 2.f;
  position = 0.5f * (ub + lb);
  position = glm::vec3(glm::inverse(view) * glm::vec4(position, 1.f));
}

} // namespace glmmd