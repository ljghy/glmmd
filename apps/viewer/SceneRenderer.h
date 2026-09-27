#ifndef GLMMD_VIEWER_SCENE_RENDERER_H_
#define GLMMD_VIEWER_SCENE_RENDERER_H_

#include "InfiniteGridRenderer.h"
#include "ModelRenderer.h"

// Owns the scene-wide pass state and targets. All color attachments share one
// depth buffer; only the opaque phase writes it.
class SceneRenderer {
public:
  SceneRenderer(int width, int height, int samples);

  void resize(int width, int height);
  void render(const std::vector<std::unique_ptr<ModelRenderer>> &models,
              InfiniteGridRenderer *grid, const glmmd::Camera &camera,
              const glmmd::DirectionalLight &light, const ShadowMap *shadowMap,
              const glm::vec4 &background, bool wireframe);

  const ogl::Texture2D &colorTexture() const {
    return *m_output.colorTextureAttachment();
  }

private:
  int m_width;
  int m_height;
  int m_samples;
  ogl::FrameBufferObject m_scene;
  ogl::FrameBufferObject m_output;
  ogl::VertexArrayObject m_fullscreenVAO;
  ogl::Shader m_composite;
};

#endif
