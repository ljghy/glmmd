#ifndef VIEWER_AXES_RENDERER_H_
#define VIEWER_AXES_RENDERER_H_

#include <opengl_framework/Common.h>

#include <glmmd/core/Camera.h>

class AxesRenderer
{
public:
    AxesRenderer(float axisLength);

    void render(const glmmd::Camera &camera);

private:
    float                   m_axisLength;
    ogl::VertexBufferObject m_VBO;
    ogl::VertexArrayObject  m_VAO;
    ogl::Shader             m_shader;
};

#endif
