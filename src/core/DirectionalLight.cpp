#include <glm/gtc/matrix_transform.hpp>

#include <glmmd/core/DirectionalLight.h>

namespace glmmd
{

const glm::mat4 &DirectionalLight::view() const { return m_view; }

const glm::mat4 &DirectionalLight::proj() const { return m_proj; }

static const glm::vec3 worldUp(0.f, 1.f, 0.f);

constexpr float boundingBoxEnlargeFactor = 1.5f;

void DirectionalLight::update()
{
    m_view = glm::lookAt(position, position + direction, worldUp);
    m_proj = glm::ortho(-extents.x, extents.x, -extents.y, extents.y,
                        -extents.z, extents.z);
}

void DirectionalLight::updateFrustum(size_t count, const glm::vec3 *points)
{
    if (count == 0)
        return;

    glm::mat4 view = glm::lookAt(glm::vec3(0.f), direction, worldUp);

    glm::vec3 p  = glm::vec3(view * glm::vec4(points[0], 1.f));
    glm::vec3 lb = p;
    glm::vec3 ub = p;

    for (size_t i = 1; i < count; ++i)
    {
        p  = glm::vec3(view * glm::vec4(points[i], 1.f));
        lb = glm::min(lb, p);
        ub = glm::max(ub, p);
    }

    extents = (boundingBoxEnlargeFactor * 0.5f) * (ub - lb);
    extents.z *= 2.f;
    position = 0.5f * (ub + lb);
    position = glm::vec3(glm::inverse(view) * glm::vec4(position, 1.f));
}

} // namespace glmmd