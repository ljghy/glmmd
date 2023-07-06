#include <glmmd/render/ModelRenderer.h>

namespace glmmd
{

#include "DefaultShaderSources.inl"
#include "SharedToonTextures.inl"

bool                      ModelRenderer::sharedToonTexturesLoaded = false;
std::array<Texture2D, 10> ModelRenderer::sharedToonTextures;

void ModelRenderer::releaseSharedToonTextures()
{
    for (auto &texture : sharedToonTextures)
        texture.destroy();
}

ModelRenderer::ModelRenderer(const ModelData           &data,
                             ModelRendererShaderSources shaderSources)
    : m_modelData(data)
    , m_renderData(data)
{
    m_VBO.create(nullptr,
                 static_cast<unsigned int>((3 + 3 + 2) * sizeof(float) *
                                           data.vertices.size()),
                 GL_DYNAMIC_DRAW);
    m_VBO.bind();

    VertexBufferLayout layout(true);
    layout.push(GL_FLOAT, 3,
                static_cast<unsigned int>(data.vertices.size() * 3));
    layout.push(GL_FLOAT, 3,
                static_cast<unsigned int>(data.vertices.size() * 3));
    layout.push(GL_FLOAT, 2,
                static_cast<unsigned int>(data.vertices.size() * 2));

    m_VAO.create();
    m_VAO.bind();
    m_VAO.addBuffer(m_VBO, layout);

    m_IBOs.resize(data.materials.size());
    size_t indexOffset = 0;
    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        m_IBOs[i].create(&data.indices[indexOffset],
                         sizeof(uint32_t) * data.materials[i].indicesCount);
        indexOffset += data.materials[i].indicesCount;
    }

    m_shader.create(shaderSources.vertShaderSrc, shaderSources.fragShaderSrc);
    m_edgeShader.create(shaderSources.edgeVertShaderSrc,
                        shaderSources.edgeFragShaderSrc);

    m_textures.resize(data.textures.size());
    for (size_t i = 0; i < data.textures.size(); ++i)
    {
        if (!data.textures[i].exists)
            continue;

        Texture2DCreateInfo info{.width  = data.textures[i].width,
                                 .height = data.textures[i].height,
                                 .data   = data.textures[i].pixels.data()};
        m_textures[i].create(info);
    }

    if (!sharedToonTexturesLoaded)
    {
        for (size_t i = 0; i < sharedToonTextures.size(); ++i)
        {
            Texture2DCreateInfo info{.width  = 32,
                                     .height = 32,
                                     .data = reinterpret_cast<unsigned char *>(
                                         sharedToonTextureData[i].data()),
                                     .dataFmt = GL_RGB};
            sharedToonTextures[i].create(info);
        }
        sharedToonTexturesLoaded = true;
    }
}

void ModelRenderer::fillBuffers() const
{
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    3 * sizeof(float) * m_renderData.positions.size(),
                    m_renderData.positions.data());

    glBufferSubData(GL_ARRAY_BUFFER,
                    3 * sizeof(float) * m_renderData.positions.size(),
                    3 * sizeof(float) * m_renderData.normals.size(),
                    m_renderData.normals.data());

    glBufferSubData(GL_ARRAY_BUFFER,
                    (3 + 3) * sizeof(float) * m_renderData.positions.size(),
                    2 * sizeof(float) * m_renderData.UVs.size(),
                    m_renderData.UVs.data());
}

void ModelRenderer::render(const Camera &camera, const Lighting &lighting)
{
    if (m_renderFlag == MODEL_RENDER_FLAG_NONE)
        return;

    m_VBO.bind();
    m_VAO.bind();
    fillBuffers();

    if (m_renderFlag & MODEL_RENDER_FLAG_MESH)
        renderMesh(camera, lighting);

    if (m_renderFlag & MODEL_RENDER_FLAG_EDGE)
        renderEdge(camera);
}

void ModelRenderer::renderMesh(const Camera &camera, const Lighting &lighting)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 proj  = camera.proj();
    glm::mat4 view  = camera.view();
    glm::mat4 MV    = view * model;
    glm::mat4 MVP   = proj * MV;

    m_shader.use();
    m_shader.setUniformMatrix4fv("u_model", &model[0][0]);
    m_shader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);

    m_shader.setUniform3fv("u_viewDir", &camera.front[0]);
    m_shader.setUniform3fv("u_lightDir", &lighting.direction[0]);
    m_shader.setUniform3fv("u_lightColor", &lighting.color[0]);
    m_shader.setUniform3fv("u_ambientColor", &lighting.ambientColor[0]);

    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        const auto &mat    = m_renderData.materials[i];
        const auto &matAdd = m_renderData.materialAdd[i];
        const auto &matMul = m_renderData.materialMul[i];

        m_modelData.materials[i].doubleSided() ? glDisable(GL_CULL_FACE)
                                               : glEnable(GL_CULL_FACE);

        m_shader.setUniform4fv("u_mat.diffuse", &mat.diffuse[0]);
        m_shader.setUniform3fv("u_mat.specular", &mat.specular[0]);
        m_shader.setUniform1f("u_mat.specularPower", mat.specularPower);
        m_shader.setUniform3fv("u_mat.ambient", &mat.ambient[0]);

        int32_t textureIndex = m_modelData.materials[i].textureIndex;
        if (textureIndex >= 0 && m_textures[textureIndex].id() != 0)
        {
            m_textures[textureIndex].bind(0);
            m_shader.setUniform1i("u_mat.hasTexture", 1);
            m_shader.setUniform4fv("u_mat.textureAdd", &matAdd.texture[0]);
            m_shader.setUniform4fv("u_mat.textureMul", &matMul.texture[0]);
            m_shader.setUniform1i("u_mat.texture", 0);
        }
        else
        {
            m_shader.setUniform1i("u_mat.hasTexture", 0);
        }

        int32_t sphereTextureIndex =
            m_modelData.materials[i].sphereTextureIndex;
        if (sphereTextureIndex >= 0 &&
            m_modelData.materials[i].sphereMode != 0 &&
            m_textures[sphereTextureIndex].id() != 0)
        {
            m_textures[sphereTextureIndex].bind(1);
            m_shader.setUniform1i("u_mat.sphereTextureMode",
                                  m_modelData.materials[i].sphereMode);
            m_shader.setUniform4fv("u_mat.sphereTextureAdd",
                                   &matAdd.sphereTexture[0]);
            m_shader.setUniform4fv("u_mat.sphereTextureMul",
                                   &matMul.sphereTexture[0]);
            m_shader.setUniform1i("u_mat.sphereTexture", 1);
        }
        else
        {
            m_shader.setUniform1i("u_mat.sphereTextureMode", 0);
        }

        int32_t toonTextureIndex = m_modelData.materials[i].toonTextureIndex;
        if (toonTextureIndex >= 0 && (m_modelData.materials[i].sharedToonFlag ||
                                      m_textures[toonTextureIndex].id() != 0))
        {
            (m_modelData.materials[i].sharedToonFlag
                 ? sharedToonTextures[toonTextureIndex]
                 : m_textures[toonTextureIndex])
                .bind(2);
            m_shader.setUniform1i("u_mat.hasToonTexture", 1);
            m_shader.setUniform4fv("u_mat.toonTextureAdd",
                                   &matAdd.toonTexture[0]);
            m_shader.setUniform4fv("u_mat.toonTextureMul",
                                   &matMul.toonTexture[0]);
            m_shader.setUniform1i("u_mat.toonTexture", 2);
        }
        else
        {
            m_shader.setUniform1i("u_mat.hasToonTexture", 0);
        }
        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

void ModelRenderer::renderEdge(const Camera &camera)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.f, 1.f);

    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 proj  = camera.proj();
    glm::mat4 view  = camera.view();
    glm::mat4 MV    = view * model;
    glm::mat4 MVP   = proj * MV;

    m_edgeShader.use();
    m_edgeShader.setUniformMatrix4fv("u_MV", &MV[0][0]);
    m_edgeShader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
    m_edgeShader.setUniform2fv(
        "u_viewportSize",
        &glm::vec2(camera.viewportWidth, camera.viewportHeight)[0]);

    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        const auto &mat = m_renderData.materials[i];

        if (!m_modelData.materials[i].renderEdge() || mat.edgeSize == 0.f ||
            mat.edgeColor.a == 0.f)
            continue;

        m_edgeShader.setUniform1f("u_edgeSize", mat.edgeSize);
        m_edgeShader.setUniform4fv("u_edgeColor", &mat.edgeColor[0]);
        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

} // namespace glmmd