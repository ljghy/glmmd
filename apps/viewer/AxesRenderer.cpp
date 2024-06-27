#include <glm/gtc/matrix_transform.hpp>

#include "AxesRenderer.h"

AxesRenderer::AxesRenderer()
{
    m_dummyVAO.create();
    const char *vertShaderSrc = R"(
        #version 330 core
        uniform mat4 u_MVP;
        out vec3 color;
        void main()
        {
            vec3 pos = vec3(0.0, 0.0, 0.0);
            int axis = gl_VertexID / 2;
            if (gl_VertexID % 2 == 1)
                pos[axis] = 1.0;
            color = vec3(0.0, 0.0, 0.0);
            color[axis] = 1.0;
            gl_Position = u_MVP * vec4(pos, 1.0);
        }
    )";
    const char *fragShaderSrc = R"(
        #version 330 core
        in vec3 color;
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(color, 1.0);
        }
    )";
    m_shader.create(vertShaderSrc, fragShaderSrc);
}

void AxesRenderer::render(const glmmd::Camera &camera)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    m_dummyVAO.bind();
    m_shader.use();
    glm::mat4 MVP = camera.proj() * camera.view() *
                    glm::scale(glm::mat4(1.f), glm::vec3(length));
    m_shader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
    glLineWidth(lineWidth);
    glDrawArrays(GL_LINES, 0, 6);
}
