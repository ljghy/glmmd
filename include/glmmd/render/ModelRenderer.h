#ifndef GLMMD_VIEWER_MODEL_RENDERER_H_
#define GLMMD_VIEWER_MODEL_RENDERER_H_

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

struct ModelRendererShaderSources
{
    const char *vertShaderSrc     = defaultVertShaderSrc;
    const char *fragShaderSrc     = defaultFragShaderSrc;
    const char *edgeVertShaderSrc = defaultEdgeVertShaderSrc;
    const char *edgeFragShaderSrc = defaultEdgeFragShaderSrc;
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
    ModelRenderer(const ModelData           &data,
                  ModelRendererShaderSources shaderSources = {});

    void render(const Camera &camera, const Lighting &lighting);

    RenderData &renderData() { return m_renderData; }

    static void releaseSharedToonTextures();

    uint32_t &renderFlag() { return m_renderFlag; }

private:
    void fillBuffers() const;
    void renderMesh(const Camera &camera, const Lighting &lighting);
    void renderEdge(const Camera &camera);

private:
    const ModelData &m_modelData;

    RenderData m_renderData;

    VertexBufferObject             m_VBO;
    VertexArrayObject              m_VAO;
    std::vector<IndexBufferObject> m_IBOs;
    Shader                         m_shader;
    Shader                         m_edgeShader;
    std::vector<Texture2D>         m_textures;

    static bool                      sharedToonTexturesLoaded;
    static std::array<Texture2D, 10> sharedToonTextures;

    uint32_t m_renderFlag = MODEL_RENDER_FLAG_MESH | MODEL_RENDER_FLAG_EDGE;
};

} // namespace glmmd

#endif