#ifndef GLMMD_CORE_DIRECTIONAL_LIGHT_H_
#define GLMMD_CORE_DIRECTIONAL_LIGHT_H_

#include <cstddef>

#include <glm/glm.hpp>

namespace glmmd
{

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambientColor;

    glm::vec3 position;
    glm::vec3 extents;

    const glm::mat4 &view() const;
    const glm::mat4 &proj() const;

    void update();

    void updateFrustum(size_t count, const glm::vec3 *points);

private:
    glm::mat4 m_view;
    glm::mat4 m_proj;
};

} // namespace glmmd

#endif
