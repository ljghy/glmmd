#ifndef VIEWER_AXES_RENDERER_H_
#define VIEWER_AXES_RENDERER_H_

#include <opengl_framework/Common.h>

#include <glmmd/core/Camera.h>

class AxesRenderer
{
public:
    AxesRenderer();

    void render(const glmmd::Camera &camera);

public:
    float length    = 100.f;
    float lineWidth = 2.f;

private:
    ogl::VertexArrayObject m_dummyVAO;
    ogl::Shader            m_shader;
};

#endif
