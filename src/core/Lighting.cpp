#include <glm/gtc/matrix_transform.hpp>

#include <glmmd/core/Lighting.h>

namespace glmmd
{

glm::mat4 Lighting::view() const
{
    return glm::lookAt(-direction * dist, glm::vec3(0.f),
                       glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 Lighting::proj() const
{
    return glm::ortho(-width, width, -width, width, nearZ, farZ);
}

} // namespace glmmd