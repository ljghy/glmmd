#ifndef GLMMD_VIEWER_MODEL_RENDERER_H_
#define GLMMD_VIEWER_MODEL_RENDERER_H_

#include <memory>

#include <opengl_framework/Common.h>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/RenderData.h>
#include <glmmd/render/Camera.h>
#include <glmmd/render/Lighting.h>

namespace glmmd
{

extern const char *defaultVertShaderSrc;
extern const char *defaultFragShaderSrc;
extern const char *defaultEdgeVertShaderSrc;
extern const char *defaultEdgeFragShaderSrc;
extern const char *defaultShadowMapVertShaderSrc;
extern const char *defaultShadowMapFragShaderSrc;

struct ModelRendererShaderSources
{
    const char *vertShaderSrc          = defaultVertShaderSrc;
    const char *fragShaderSrc          = defaultFragShaderSrc;
    const char *edgeVertShaderSrc      = defaultEdgeVertShaderSrc;
    const char *edgeFragShaderSrc      = defaultEdgeFragShaderSrc;
    const char *shadowMapVertShaderSrc = defaultShadowMapVertShaderSrc;
    const char *shadowMapFragShaderSrc = defaultShadowMapFragShaderSrc;
};

enum ModelRenderFlag : uint32_t
{
    MODEL_RENDER_FLAG_NONE = 0,
    MODEL_RENDER_FLAG_MESH = 1 << 0,
    MODEL_RENDER_FLAG_EDGE = 1 << 1,
};

class ModelRenderer
{
public:
    ModelRenderer(const std::shared_ptr<const ModelData> &data,
                  ModelRendererShaderSources              shaderSources = {});

    void renderShadowMap(const Lighting &lighting) const;
    void render(const Camera &camera, const Lighting &lighting,
                const Texture2D *shadowMap = nullptr) const;

    RenderData       &renderData() { return m_renderData; }
    const RenderData &renderData() const { return m_renderData; }

    static void releaseSharedToonTextures();

    uint32_t &renderFlag() { return m_renderFlag; }

private:
    void initBuffers();
    void initTextures();
    void initSharedToonTextures();

    void fillBuffers() const;
    void renderMesh(const Camera &camera, const Lighting &lighting,
    const Texture2D* shadowMap) const;
    void renderEdge(const Camera &camera) const;

private:
    std::shared_ptr<const ModelData> m_modelData;

    RenderData m_renderData;

    VertexBufferObject             m_VBO;
    VertexArrayObject              m_VAO;
    std::vector<IndexBufferObject> m_IBOs;

    Shader m_shader;
    Shader m_edgeShader;
    Shader m_shadowMapShader;

    std::vector<Texture2D> m_textures;

    static bool                      sharedToonTexturesLoaded;
    static std::array<Texture2D, 10> sharedToonTextures;

    uint32_t m_renderFlag = MODEL_RENDER_FLAG_MESH | MODEL_RENDER_FLAG_EDGE;
};

} // namespace glmmd

#endif