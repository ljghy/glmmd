#include "AxesRenderer.h"

AxesRenderer::AxesRenderer(float axisLength)
    : m_axisLength(axisLength)
{
    float vertices[] = {0.f, 0.f, 0.f, axisLength, 0.f,        0.f,
                        0.f, 0.f, 0.f, 0.f,        axisLength, 0.f,
                        0.f, 0.f, 0.f, 0.f,        0.f,        axisLength};

    m_VBO.create(vertices, sizeof(vertices));
    m_VBO.bind();
    ogl::VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);
    m_VAO.create();
    m_VAO.bind();
    m_VAO.addBuffer(m_VBO, layout);

    const char *vertShaderSrc = R"(
        #version 330 core
        uniform mat4 u_MVP;
        layout(location = 0) in vec3 aPos;
        out vec3 color;
        void main()
        {
            color = vec3(0.0, 0.0, 0.0);
            color[gl_VertexID / 2] = 1.0;
            gl_Position = u_MVP * vec4(aPos, 1.0);
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
    glEnable(GL_BLEND);

    m_VBO.bind();
    m_VAO.bind();
    m_shader.use();
    glm::mat4 MVP = camera.proj() * camera.view();
    m_shader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
    glLineWidth(2.f);
    glDrawArrays(GL_LINES, 0, 6);
}
