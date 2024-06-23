#ifndef VIEWER_GRID_RENDERER_H_
#define VIEWER_GRID_RENDERER_H_

#include <opengl_framework/Common.h>

#include <glmmd/core/Camera.h>

class GridRenderer
{
public:
    GridRenderer(int size, float step);

    void render(const glmmd::Camera &camera);

private:
    int                     m_nV;
    ogl::VertexBufferObject m_VBO;
    ogl::VertexArrayObject  m_VAO;
    ogl::Shader             m_shader;
};

#endif
