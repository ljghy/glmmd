#ifndef GLMMD_VIEWER_SHADOW_MAP_H_
#define GLMMD_VIEWER_SHADOW_MAP_H_

#include <memory>
#include <vector>

#include <glmmd/core/Camera.h>
#include <glmmd/core/DirectionalLight.h>
#include <opengl_framework/Common.h>

class ModelRenderer;

struct ShadowSettings {
  float distance = 50.f;
  int filterRadius = 2;       // 5x5 or 7x7 tent PCF
  float constantBias = 0.25f; // Bias controls are in world-space shadow texels.
  float slopeBias = 1.f;
  float normalBias = 0.5f;
  float alphaCutoff = 0.5f;
};

class ShadowMap {
public:
  ShadowMap(int width, int height);
  void resize(int width, int height);
  void update(const glmmd::Camera &camera, glmmd::DirectionalLight &light,
              const std::vector<std::unique_ptr<ModelRenderer>> &models);
  void render(const glmmd::DirectionalLight &light,
              const std::vector<std::unique_ptr<ModelRenderer>> &models) const;
  void bindReceiver(const ogl::Shader &shader,
                    const glmmd::Camera &camera) const;

  int width() const { return m_target.depthTextureAttachment()->width(); }
  int height() const { return m_target.depthTextureAttachment()->height(); }
  ShadowSettings settings;

private:
  ogl::FrameBufferObject m_target;
  int m_maxSize;
  glm::vec3 m_lastDirection{0.f, -1.f, 0.f};
  float m_texelWorld = 1.f;
  float m_depthSpan = 1.f;
  glm::vec2 m_fadeRange{0.f, 1.f};
};

#endif
