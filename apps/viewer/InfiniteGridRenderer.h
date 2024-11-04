#ifndef VIEWER_GRID_RENDERER_H_
#define VIEWER_GRID_RENDERER_H_

#include <opengl_framework/Common.h>

#include <glmmd/core/Camera.h>

class InfiniteGridRenderer
{
public:
    InfiniteGridRenderer();

    void render(const glmmd::Camera &camera);

public:
    float     gridSize     = 5.f;
    float     lineWidth    = 1.f;
    float     falloffDepth = 200.f;
    glm::vec3 color        = glm::vec3(0.6, 0.6, 0.6);
    int       showAxes     = 1;

private:
    ogl::VertexArrayObject m_dummyVAO;
    ogl::Shader            m_shader;
};

#endif
