#include "InfiniteGridRenderer.h"

InfiniteGridRenderer::InfiniteGridRenderer()
{
    m_dummyVAO.create();
    const char *vertShaderSrc = R"(
        #version 330 core
        uniform mat4 u_VP;
        uniform mat4 u_invVP;
        vec2 ndcPos[3] = vec2[](
            vec2(-1.0, -1.0),
            vec2( 3.0, -1.0),
            vec2(-1.0,  3.0)
        );
        out vec3 nearWorld;
        out vec3 farWorld;
        void main()
        {
            vec2 pos = ndcPos[gl_VertexID];
            vec4 near = u_invVP * vec4(pos, -1.0, 1.0);
            nearWorld = near.xyz / near.w;
            vec4 far = u_invVP * vec4(pos, 1.0, 1.0);
            farWorld = far.xyz / far.w;
            float t = nearWorld.y / (nearWorld.y - farWorld.y);
            vec3 worldPos = mix(nearWorld, farWorld, t);
            vec4 clipPos = u_VP * vec4(worldPos, 1.0);
            float depth = clipPos.z / clipPos.w;
            gl_Position = vec4(pos, depth, 1.0);
        }
    )";
    const char *fragShaderSrc = R"(
        #version 330 core
        uniform mat4 u_V;
        uniform float u_gridSize;
        uniform float u_lineWidth;
        uniform float u_falloffDepth;
        uniform vec3 u_color;
        uniform int u_showAxes;
        in vec3 nearWorld;
        in vec3 farWorld;
        out vec4 FragColor;
        float smoothFunc(float x) {
            return x * x * (3.0 - 2.0 * x);
        }
        void main()
        {
            float t = nearWorld.y / (nearWorld.y - farWorld.y);
            if (t < 0.0 || t > 1.0)
                discard;
            vec3 worldPos = mix(nearWorld, farWorld, t);
            vec4 viewPos = u_V * vec4(worldPos, 1.0);
            float linearDepth = -viewPos.z / viewPos.w;
            if (linearDepth > u_falloffDepth)
                discard;
            vec2 diff = fwidth(worldPos.xz / u_gridSize);
            vec2 lineWidth = u_lineWidth * diff;
            float indX = worldPos.x / u_gridSize;
            int offsetX = int(round(indX));
            indX -= floor(indX);
            indX = min(indX, 1.0 - indX) / lineWidth.x;
            float indZ = worldPos.z / u_gridSize;
            int offsetZ = int(round(indZ));
            indZ -= floor(indZ);
            indZ = min(indZ, 1.0 - indZ) / lineWidth.y;
            if (indX > 1.0 && indZ > 1.0)
                discard;
            float ind = min(indX, indZ);
            float alpha = smoothFunc(1.0 - ind);
            if (linearDepth > 0.0)
                alpha *= smoothFunc(1.0 - linearDepth / u_falloffDepth);
            vec3 color = u_color;
            if ((indX < 1.0 && abs(offsetX) % 10 == 0) || (indZ < 1.0 && abs(offsetZ) % 10 == 0))
                color *= 1.5;
            if (u_showAxes > 0) {
                if (indX < 1.0 && offsetX == 0 && worldPos.z > 0.0)
                    color = vec3(0.0, 0.0, 1.0);
                else if (indZ < 1.0 && offsetZ == 0 && worldPos.x > 0.0)
                    color = vec3(1.0, 0.0, 0.0);
            }
            FragColor = vec4(color, alpha);
        }
    )";
    m_shader.create(vertShaderSrc, fragShaderSrc);
}

void InfiniteGridRenderer::render(const glmmd::Camera &camera)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_dummyVAO.bind();
    m_shader.use();
    glm::mat4 V     = camera.view();
    glm::mat4 VP    = camera.proj() * V;
    glm::mat4 invVP = glm::inverse(VP);
    m_shader.setUniformMatrix4fv("u_V", &V[0][0]);
    m_shader.setUniformMatrix4fv("u_VP", &VP[0][0]);
    m_shader.setUniformMatrix4fv("u_invVP", &invVP[0][0]);
    m_shader.setUniform1f("u_gridSize", gridSize);
    m_shader.setUniform1f("u_lineWidth", lineWidth);
    m_shader.setUniform1f("u_falloffDepth", falloffDepth);
    m_shader.setUniform3fv("u_color", &color[0]);
    m_shader.setUniform1i("u_showAxes", showAxes);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
