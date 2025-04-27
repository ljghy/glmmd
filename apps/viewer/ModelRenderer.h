#ifndef GLMMD_VIEWER_MODEL_RENDERER_H_
#define GLMMD_VIEWER_MODEL_RENDERER_H_

#include <memory>

#include <opengl_framework/Common.h>

#include <glmmd/core/Camera.h>
#include <glmmd/core/DirectionalLight.h>
#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelRenderData.h>

extern const char *defaultVertShaderSrc;
extern const char *defaultFragShaderSrc;
extern const char *defaultEdgeVertShaderSrc;
extern const char *defaultEdgeFragShaderSrc;
extern const char *defaultShadowMapVertShaderSrc;
extern const char *defaultShadowMapFragShaderSrc;
extern const char *defaultGroundShadowVertShaderSrc;
extern const char *defaultGroundShadowFragShaderSrc;

struct ModelRendererShaderSources
{
    const char *vertShaderSrc             = defaultVertShaderSrc;
    const char *fragShaderSrc             = defaultFragShaderSrc;
    const char *edgeVertShaderSrc         = defaultEdgeVertShaderSrc;
    const char *edgeFragShaderSrc         = defaultEdgeFragShaderSrc;
    const char *shadowMapVertShaderSrc    = defaultShadowMapVertShaderSrc;
    const char *shadowMapFragShaderSrc    = defaultShadowMapFragShaderSrc;
    const char *groundShadowVertShaderSrc = defaultGroundShadowVertShaderSrc;
    const char *groundShadowFragShaderSrc = defaultGroundShadowFragShaderSrc;
};

enum ModelRenderFlag : uint32_t
{
    MODEL_RENDER_FLAG_EMPTY         = 0,
    MODEL_RENDER_FLAG_HIDE          = 1 << 0,
    MODEL_RENDER_FLAG_MESH          = 1 << 1,
    MODEL_RENDER_FLAG_EDGE          = 1 << 2,
    MODEL_RENDER_FLAG_GROUND_SHADOW = 1 << 3,
};

class ModelRenderer
{
public:
    ModelRenderer(const std::shared_ptr<const glmmd::ModelData> &data,
                  const ModelRendererShaderSources &shaderSources = {});

    void fillBuffers() const;

    void renderShadowMap(const glmmd::DirectionalLight &light) const;
    void render(const glmmd::Camera           &camera,
                const glmmd::DirectionalLight &light,
                const ogl::Texture2D          *shadowMap = nullptr) const;

    glmmd::ModelRenderData       &renderData() { return m_renderData; }
    const glmmd::ModelRenderData &renderData() const { return m_renderData; }

    static void releaseSharedToonTextures();

    uint32_t &renderFlag() { return m_renderFlag; }

private:
    void initBuffers();
    void initTextures();
    void initSharedToonTextures();
    void initShaders(const ModelRendererShaderSources &shaderSources);

    void renderMesh(const glmmd::Camera           &camera,
                    const glmmd::DirectionalLight &light,
                    const ogl::Texture2D          *shadowMap) const;
    void renderEdge(const glmmd::Camera &camera) const;
    void renderGroundShadow(const glmmd::Camera           &camera,
                            const glmmd::DirectionalLight &light) const;

private:
    std::shared_ptr<const glmmd::ModelData> m_modelData;

    glmmd::ModelRenderData m_renderData;

    ogl::VertexBufferObject m_VBO;
    ogl::VertexArrayObject  m_VAO;
    ogl::IndexBufferObject  m_IBO;

    ogl::Shader m_shader;
    ogl::Shader m_edgeShader;
    ogl::Shader m_shadowMapShader;
    ogl::Shader m_groundShadowShader;

    std::vector<ogl::Texture2D> m_textures;

    static bool                           sharedToonTexturesLoaded;
    static std::array<ogl::Texture2D, 10> sharedToonTextures;

    uint32_t m_renderFlag = MODEL_RENDER_FLAG_MESH | MODEL_RENDER_FLAG_EDGE |
                            MODEL_RENDER_FLAG_GROUND_SHADOW;
};

#endif