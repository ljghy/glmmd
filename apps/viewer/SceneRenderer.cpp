#include "SceneRenderer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr const char *fullscreenVertex = R"(
#version 410 core
void main() {
    vec2 positions[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
    gl_Position = vec4(positions[gl_VertexID], 0, 1);
}
)";

constexpr const char *compositeFragment = R"(
#ifdef MULTISAMPLE
uniform sampler2DMS u_opaque;
uniform sampler2DMS u_accumulation;
uniform sampler2DMS u_revealage;
#else
uniform sampler2D u_opaque;
uniform sampler2D u_accumulation;
uniform sampler2D u_revealage;
#endif
uniform int u_samples;
out vec4 FragColor;

vec3 linearToDisplay(vec3 color) {
    color = max(color, vec3(0.0));
    return mix(12.92 * color, 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055,
               step(vec3(0.0031308), color));
}

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 resolved = vec4(0.0);
    for (int sampleIndex = 0; sampleIndex < u_samples; ++sampleIndex) {
#ifdef MULTISAMPLE
        int index = sampleIndex;
#else
        int index = 0; // mip level for single-sample textures
#endif
        vec4 opaque = texelFetch(u_opaque, pixel, index);
        vec4 accum = texelFetch(u_accumulation, pixel, index);
        float reveal = clamp(texelFetch(u_revealage, pixel, index).r, 0.0, 1.0);
        vec3 transparent = accum.rgb / max(accum.a, 1e-6);
        resolved += vec4(transparent * (1.0 - reveal) + opaque.rgb * reveal,
                         (1.0 - reveal) + opaque.a * reveal);
    }
    resolved /= float(u_samples);
    // ImGui's image shader uses straight-alpha blending. Resolve in linear,
    // premultiplied space, then unpremultiply before display encoding.
    FragColor = vec4(linearToDisplay(resolved.a > 0.0
                        ? resolved.rgb / resolved.a : vec3(0.0)), resolved.a);
}
)";

float displayToLinear(float value) {
  return value <= 0.04045f ? value / 12.92f
                           : std::pow((value + 0.055f) / 1.055f, 2.4f);
}
} // namespace

SceneRenderer::SceneRenderer(int width, int height, int samples)
    : m_width(width), m_height(height), m_samples(1) {
  GLint maxSamples;
  glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
  samples = std::clamp(samples, 1, maxSamples);
  while (m_samples * 2 <= samples)
    m_samples *= 2;

  m_scene.create();
  ogl::Texture2DCreateInfo info;
  info.width = width;
  info.height = height;
  info.samples = m_samples;
  info.internalFmt = GL_RGBA16F;
  info.dataFmt = GL_RGBA;
  info.dataType = GL_FLOAT;
  info.minFilterMode = GL_NEAREST;
  info.magFilterMode = GL_NEAREST;
  info.wrapModeS = GL_CLAMP_TO_EDGE;
  info.wrapModeT = GL_CLAMP_TO_EDGE;
  m_scene.attachColorTexture(std::make_unique<ogl::Texture2D>(info), 0);
  if (m_samples > 1) {
    m_scene.colorTextureAttachment()->bind();
    glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_SAMPLES,
                             &m_samples);
    info.samples = m_samples;
  }
  m_scene.attachColorTexture(std::make_unique<ogl::Texture2D>(info), 1);
  info.internalFmt = GL_R16F;
  info.dataFmt = GL_RED;
  m_scene.attachColorTexture(std::make_unique<ogl::Texture2D>(info), 2);

  ogl::RenderBufferObjectCreateInfo depth;
  depth.width = width;
  depth.height = height;
  depth.samples = m_samples;
  depth.internalFmt = GL_DEPTH_COMPONENT24;
  m_scene.attachDepthRenderBuffer(
      std::make_unique<ogl::RenderBufferObject>(depth));

  m_output.create();
  info.samples = 1;
  info.internalFmt = GL_RGBA8;
  info.dataFmt = GL_RGBA;
  info.dataType = GL_UNSIGNED_BYTE;
  info.minFilterMode = GL_LINEAR;
  info.magFilterMode = GL_LINEAR;
  m_output.attachColorTexture(std::make_unique<ogl::Texture2D>(info));
  if (!m_scene.isComplete() || !m_output.isComplete())
    throw std::runtime_error("Failed to create scene render targets.");

  m_fullscreenVAO.create();
  std::string source = "#version 410 core\n";
  if (m_samples > 1)
    source += "#define MULTISAMPLE\n";
  source += compositeFragment;
  m_composite.create(fullscreenVertex, source.c_str());
}

void SceneRenderer::resize(int width, int height) {
  m_width = width;
  m_height = height;
  m_scene.resize(width, height);
  m_output.resize(width, height);
}

void SceneRenderer::render(
    const std::vector<std::unique_ptr<ModelRenderer>> &models,
    InfiniteGridRenderer *grid, const glmmd::Camera &camera,
    const glmmd::DirectionalLight &light, const ShadowMap *shadowMap,
    const glm::vec4 &background, bool wireframe) {
  m_scene.bind();
  glViewport(0, 0, m_width, m_height);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  if (m_samples > 1) {
    glEnable(GL_SAMPLE_SHADING);
    glMinSampleShading(1.f);
  } else {
    glDisable(GL_SAMPLE_SHADING);
  }
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  const float alpha = std::clamp(background.a, 0.f, 1.f);
  const glm::vec4 clear(displayToLinear(background.r) * alpha,
                        displayToLinear(background.g) * alpha,
                        displayToLinear(background.b) * alpha, alpha);
  glClearBufferfv(GL_COLOR, 0, &clear[0]);
  const float farDepth = 1.f;
  glClearBufferfv(GL_DEPTH, 0, &farDepth);

  glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
  for (const auto &model : models)
    model->render(SurfacePass::Opaque, camera, light, shadowMap);
  if (grid)
    grid->render(SurfacePass::Opaque, camera);

  const GLenum oitBuffers[] = {GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
  glDrawBuffers(2, oitBuffers);
  const float zero[4] = {};
  const float one[4] = {1.f, 1.f, 1.f, 1.f};
  glClearBufferfv(GL_COLOR, 0, zero);
  glClearBufferfv(GL_COLOR, 1, one);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunci(0, GL_ONE, GL_ONE);
  glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
  for (const auto &model : models)
    model->render(SurfacePass::Transparent, camera, light, shadowMap);
  if (grid)
    grid->render(SurfacePass::Transparent, camera);

  m_output.bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_SAMPLE_SHADING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  m_composite.use();
  m_composite.setUniform1i("u_opaque", 0);
  m_composite.setUniform1i("u_accumulation", 1);
  m_composite.setUniform1i("u_revealage", 2);
  m_composite.setUniform1i("u_samples", m_samples);
  for (unsigned int i = 0; i < 3; ++i)
    m_scene.colorTextureAttachment(i)->bind(i);
  m_fullscreenVAO.bind();
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDepthMask(GL_TRUE);
  m_output.unbind();
}
