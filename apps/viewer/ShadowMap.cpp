#include "ShadowMap.h"
#include "ModelRenderer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

ShadowMap::ShadowMap(int width, int height) {
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxSize);
  ogl::Texture2DCreateInfo info;
  info.width = std::clamp(width, 64, m_maxSize);
  info.height = std::clamp(height, 64, m_maxSize);
  info.internalFmt = GL_DEPTH_COMPONENT32F;
  info.dataFmt = GL_DEPTH_COMPONENT;
  info.dataType = GL_FLOAT;
  info.wrapModeS = info.wrapModeT = GL_CLAMP_TO_BORDER;
  info.minFilterMode = info.magFilterMode = GL_LINEAR;
  auto texture = std::make_unique<ogl::Texture2D>(info);
  texture->bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  const float border[] = {1.f, 1.f, 1.f, 1.f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  m_target.create();
  m_target.attachDepthTexture(std::move(texture));
  m_target.bind();
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  m_target.unbind();
  if (!m_target.isComplete())
    throw std::runtime_error("Failed to create shadow map.");
}

void ShadowMap::resize(int width, int height) {
  m_target.resize(std::clamp(width, 64, m_maxSize),
                  std::clamp(height, 64, m_maxSize));
}

void ShadowMap::update(
    const glmmd::Camera &camera, glmmd::DirectionalLight &light,
    const std::vector<std::unique_ptr<ModelRenderer>> &models) {
  if (glm::dot(light.direction, light.direction) > 1e-8f)
    m_lastDirection = glm::normalize(light.direction);
  light.direction = m_lastDirection;
  const glm::vec3 up = std::abs(light.direction.y) > 0.99f
                           ? glm::vec3(0.f, 0.f, 1.f)
                           : glm::vec3(0.f, 1.f, 0.f);
  const glm::mat4 orientation =
      glm::lookAt(glm::vec3(0.f), light.direction, up);

  const float near = camera.zNear;
  const float far =
      std::min(camera.zFar, near + std::max(settings.distance, 0.1f));
  const float aspect = float(camera.viewportWidth) / camera.viewportHeight;
  const float halfHeight = camera.projType == glmmd::Camera::Perspective
                               ? far * std::tan(camera.fov * 0.5f)
                               : camera.width * 0.5f / aspect;
  const float halfWidth = halfHeight * aspect;
  const float halfDepth = (far - near) * 0.5f;
  // Compute the sphere size from projection parameters, not rotated world
  // corners: camera motion must not change shadow texel size.
  float radius =
      std::ceil(std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight +
                          halfDepth * halfDepth) *
                16.f) /
      16.f;
  const float padding = settings.filterRadius + 2.f + settings.normalBias;
  radius /= 1.f - 2.f * padding / std::min(width(), height());
  const glm::vec2 texel(2.f * radius / width(), 2.f * radius / height());
  m_texelWorld = std::max(texel.x, texel.y);
  glm::vec3 center = glm::vec3(
      orientation *
      glm::vec4(camera.position() + camera.front() * (near + far) * 0.5f, 1.f));
  center.x = std::round(center.x / texel.x) * texel.x;
  center.y = std::round(center.y / texel.y) * texel.y;

  glm::vec3 corners[8];
  camera.getFrustumCorners(corners, near, far);
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();
  for (const auto &corner : corners) {
    const float z = (orientation * glm::vec4(corner, 1.f)).z;
    minZ = std::min(minZ, z);
    maxZ = std::max(maxZ, z);
  }
  // Include animated, off-camera casters that overlap the receiver footprint
  // along the light direction. Whole-model bounds are conservative.
  for (const auto &model : models) {
    glm::vec3 lower, upper;
    if (model->shadowBounds(orientation, lower, upper) &&
        lower.x <= center.x + radius && upper.x >= center.x - radius &&
        lower.y <= center.y + radius && upper.y >= center.y - radius) {
      minZ = std::min(minZ, lower.z);
      maxZ = std::max(maxZ, upper.z);
    }
  }
  // Quantized, padded depth bounds avoid tiny animation-driven range changes.
  const float depthPadding = std::max(1.f, m_texelWorld * 4.f);
  minZ = std::floor(minZ - depthPadding);
  maxZ = std::ceil(maxZ + depthPadding);
  center.z = (minZ + maxZ) * 0.5f;
  m_depthSpan = maxZ - minZ;
  light.position =
      glm::vec3(glm::inverse(orientation) * glm::vec4(center, 1.f));
  light.extents = glm::vec3(radius, radius, m_depthSpan * 0.5f);
  light.update();
  m_fadeRange = glm::vec2(far - (far - near) * 0.1f, far);
}

void ShadowMap::render(
    const glmmd::DirectionalLight &light,
    const std::vector<std::unique_ptr<ModelRenderer>> &models) const {
  m_target.bind();
  glViewport(0, 0, width(), height());
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_SAMPLE_SHADING);
  glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glClearDepth(1.0);
  glClear(GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for (const auto &model : models)
    model->renderShadowMap(light, settings.alphaCutoff);
  m_target.unbind();
}

void ShadowMap::bindReceiver(const ogl::Shader &shader,
                             const glmmd::Camera &camera) const {
  m_target.depthTextureAttachment()->bind(3);
  shader.setUniform1f("u_shadowTexelWorld", m_texelWorld);
  shader.setUniform1f("u_shadowDepthSpan", m_depthSpan);
  const glm::vec3 bias(settings.constantBias, settings.slopeBias,
                       settings.normalBias);
  shader.setUniform3fv("u_shadowBias", &bias[0]);
  shader.setUniform1i("u_shadowRadius", settings.filterRadius);
  shader.setUniform2fv("u_shadowFade", &m_fadeRange[0]);
  const auto &view = camera.view();
  const glm::vec4 viewDepth(-view[0][2], -view[1][2], -view[2][2], -view[3][2]);
  shader.setUniform4fv("u_shadowViewDepth", &viewDepth[0]);
}
