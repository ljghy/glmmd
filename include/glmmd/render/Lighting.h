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
};

} // namespace glmmd

#endif
