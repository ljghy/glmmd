#include "GridRenderer.h"

GridRenderer::GridRenderer(int size, float step)
    : m_nV((2 * size + 1) * 4)
{
    float *vertices = new float[3 * m_nV];

    constexpr float y = 0.f;

    int offset{};
    for (int i = -size; i <= size; ++i, offset += 6)
    {
        vertices[offset + 0] = i * step;
        vertices[offset + 1] = y;
        vertices[offset + 2] = -size * step;
        vertices[offset + 3] = i * step;
        vertices[offset + 4] = y;
        vertices[offset + 5] = size * step;
    }

    for (int i = -size; i <= size; ++i, offset += 6)
    {
        vertices[offset + 0] = -size * step;
        vertices[offset + 1] = y;
        vertices[offset + 2] = i * step;
        vertices[offset + 3] = size * step;
        vertices[offset + 4] = y;
        vertices[offset + 5] = i * step;
    }

    m_VBO.create(vertices, sizeof(float) * 3 * m_nV);
    delete[] vertices;

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
        void main()
        {
            gl_Position = u_MVP * vec4(aPos, 1.0);
        }
    )";
    const char *fragShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(0.6, 0.6, 0.6, 1.0);
        }
    )";
    m_shader.create(vertShaderSrc, fragShaderSrc);
}

void GridRenderer::render(const glmmd::Camera &camera)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    m_VBO.bind();
    m_VAO.bind();
    m_shader.use();
    glm::mat4 MVP = camera.proj() * camera.view();
    m_shader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
    glLineWidth(1.f);
    glDrawArrays(GL_LINES, 0, m_nV);
}
