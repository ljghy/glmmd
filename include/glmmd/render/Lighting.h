#ifndef GLMMD_VIEWER_LIGHTING_H_
#define GLMMD_VIEWER_LIGHTING_H_

#include <glm/glm.hpp>

namespace glmmd
{

struct Lighting
{
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambientColor;

    float width = 20.f;
    float nearZ = 0.1f;
    float farZ  = 50.f;
    float dist  = 20.f;

    glm::mat4 view() const;
    glm::mat4 proj() const;
};

} // namespace glmmd

#endif
