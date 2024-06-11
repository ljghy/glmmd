#include <mutex>

#include <glmmd/render/ModelRenderer.h>

namespace glmmd
{

#include "DefaultShaderSources.inl"
#include "SharedToonTextures.inl"

std::mutex sharedToonTexturesInitMutex;

bool                           ModelRenderer::sharedToonTexturesLoaded = false;
std::array<ogl::Texture2D, 10> ModelRenderer::sharedToonTextures;

void ModelRenderer::releaseSharedToonTextures()
{
    std::lock_guard<std::mutex> lock(sharedToonTexturesInitMutex);
    if (!sharedToonTexturesLoaded)
        return;
    for (auto &texture : sharedToonTextures)
        texture.destroy();
    sharedToonTexturesLoaded = false;
}

ModelRenderer::ModelRenderer(const std::shared_ptr<const ModelData> &data,
                             const ModelRendererShaderSources &shaderSources)
    : m_modelData(data)
    , m_renderData(data)
{
    initBuffers();
    initTextures();
    initSharedToonTextures();
    initShaders(shaderSources);
}

void ModelRenderer::initBuffers()
{
    m_VBO.create(nullptr,
                 static_cast<unsigned int>(sizeof(float) *
                                           m_renderData.vertexBuffer.size()),
                 GL_DYNAMIC_DRAW);
    m_VBO.bind();

    ogl::VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);
    layout.push(GL_FLOAT, 3);
    layout.push(GL_FLOAT, 2);
    for (uint8_t i = 0; i < m_modelData->info.additionalUVNum; ++i)
        layout.push(GL_FLOAT, 4);

    m_VAO.create();
    m_VAO.bind();
    m_VAO.addBuffer(m_VBO, layout);

    m_IBOs.resize(m_modelData->materials.size());
    size_t indexOffset = 0;
    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        m_IBOs[i].create(&m_modelData->indices[indexOffset],
                         sizeof(uint32_t) *
                             m_modelData->materials[i].indicesCount);
        indexOffset += m_modelData->materials[i].indicesCount;
    }
}

void ModelRenderer::initTextures()
{
    m_textures.resize(m_modelData->textures.size());
    for (size_t i = 0; i < m_modelData->textures.size(); ++i)
    {
        const auto &tex = m_modelData->textures[i];
        if (!tex.exists)
            continue;

        ogl::Texture2DCreateInfo info;
        info.width         = tex.width;
        info.height        = tex.height;
        info.data          = tex.data.get();
        info.genMipmaps    = true;
        info.internalFmt   = GL_SRGB_ALPHA;
        info.dataFmt       = GL_RGBA;
        info.dataType      = GL_UNSIGNED_BYTE;
        info.wrapModeS     = GL_CLAMP_TO_EDGE;
        info.wrapModeT     = GL_CLAMP_TO_EDGE;
        info.minFilterMode = GL_LINEAR_MIPMAP_LINEAR;
        info.magFilterMode = GL_LINEAR;

        m_textures[i].create(info);
    }
}

void ModelRenderer::initSharedToonTextures()
{
    std::lock_guard<std::mutex> lock(sharedToonTexturesInitMutex);
    if (!sharedToonTexturesLoaded)
    {
        for (size_t i = 0; i < sharedToonTextures.size(); ++i)
        {
            ogl::Texture2DCreateInfo info;
            info.width  = 32;
            info.height = 32;
            info.data   = reinterpret_cast<unsigned char *>(
                sharedToonTextureData[i].data());
            info.genMipmaps  = false;
            info.internalFmt = GL_SRGB;
            info.dataFmt     = GL_RGB;
            info.wrapModeS   = GL_CLAMP_TO_EDGE;
            info.wrapModeT   = GL_CLAMP_TO_EDGE;
            sharedToonTextures[i].create(info);
        }
        sharedToonTexturesLoaded = true;
    }
}

void ModelRenderer::initShaders(const ModelRendererShaderSources &shaderSources)
{
    std::string vertShaderSrc = shaderSources.vertShaderSrc;
    std::string fragShaderSrc = shaderSources.fragShaderSrc;

    if (m_modelData->info.additionalUVNum > 0)
    {
        std::string vertShaderAdditionalUVLayout;
        std::string vertShaderAdditionalUVOut;
        std::string vertShaderAdditionalUVV2F;
        std::string fragShaderAdditionalUVIn = "#define USE_ADDITIONAL_UV\n";
        for (uint8_t i = 0; i < m_modelData->info.additionalUVNum; ++i)
        {
            auto j = std::to_string(i + 1);
            vertShaderAdditionalUVLayout +=
                "layout(location = " + std::to_string(i + 3) +
                ") in vec4 aAdditionalUV" + j + ";\n";
            vertShaderAdditionalUVOut += "out vec4 additionalUV" + j + ";\n";
            vertShaderAdditionalUVV2F +=
                "additionalUV" + j + " = aAdditionalUV" + j + ";\n";
            fragShaderAdditionalUVIn += "in vec4 additionalUV" + j + ";\n";
        }
        vertShaderSrc.replace(vertShaderSrc.find("///ADDITIONAL_UV_LAYOUT///"),
                              sizeof("///ADDITIONAL_UV_LAYOUT///") - 1,
                              vertShaderAdditionalUVLayout);
        vertShaderSrc.replace(vertShaderSrc.find("///ADDITIONAL_UV_OUT///"),
                              sizeof("///ADDITIONAL_UV_OUT///") - 1,
                              vertShaderAdditionalUVOut);
        vertShaderSrc.replace(vertShaderSrc.find("///ADDITIONAL_UV_V2F///"),
                              sizeof("///ADDITIONAL_UV_V2F///") - 1,
                              vertShaderAdditionalUVV2F);
        fragShaderSrc.replace(fragShaderSrc.find("///ADDITIONAL_UV_IN///"),
                              sizeof("///ADDITIONAL_UV_IN///") - 1,
                              fragShaderAdditionalUVIn);
    }

    m_shader.create(vertShaderSrc.c_str(), fragShaderSrc.c_str());
    m_edgeShader.create(shaderSources.edgeVertShaderSrc,
                        shaderSources.edgeFragShaderSrc);
    m_shadowMapShader.create(shaderSources.shadowMapVertShaderSrc,
                             shaderSources.shadowMapFragShaderSrc);
    m_groundShadowShader.create(shaderSources.groundShadowVertShaderSrc,
                                shaderSources.groundShadowFragShaderSrc);
}

void ModelRenderer::fillBuffers() const
{
    m_VBO.bind();
    m_VAO.bind();
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * m_renderData.vertexBuffer.size(),
                 m_renderData.vertexBuffer.data(), GL_DYNAMIC_DRAW);
}

void ModelRenderer::render(const Camera &camera, const Lighting &lighting,
                           const ogl::Texture2D *shadowMap) const
{
    if (m_renderFlag == MODEL_RENDER_FLAG_NONE)
        return;

    m_VBO.bind();
    m_VAO.bind();

    if (m_renderFlag & MODEL_RENDER_FLAG_MESH)
        renderMesh(camera, lighting, shadowMap);

    if (m_renderFlag & MODEL_RENDER_FLAG_EDGE)
        renderEdge(camera);

    if (m_renderFlag & MODEL_RENDER_FLAG_GROUND_SHADOW)
        renderGroundShadow(camera, lighting);
}

void ModelRenderer::renderMesh(const Camera &camera, const Lighting &lighting,
                               const ogl::Texture2D *shadowMap) const
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
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

    m_shader.setUniformMatrix4fv("u_lightVP",
                                 &(lighting.proj() * lighting.view())[0][0]);

    m_shader.setUniform1i("u_hasShadowMap", shadowMap != nullptr);
    if (shadowMap != nullptr)
    {
        shadowMap->bind(3);
        m_shader.setUniform1i("u_shadowMap", 3);
    }

    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        const auto &mat    = m_renderData.materials[i];
        const auto &matAdd = m_renderData.materialAdd[i];
        const auto &matMul = m_renderData.materialMul[i];

        if (mat.diffuse.a == 0.f)
            continue;

        m_modelData->materials[i].doubleSided() ? glDisable(GL_CULL_FACE)
                                                : glEnable(GL_CULL_FACE);

        m_shader.setUniform4fv("u_mat.diffuse", &mat.diffuse[0]);
        m_shader.setUniform3fv("u_mat.specular", &mat.specular[0]);
        m_shader.setUniform1f("u_mat.specularPower", mat.specularPower);
        m_shader.setUniform3fv("u_mat.ambient", &mat.ambient[0]);

        int32_t textureIndex = m_modelData->materials[i].textureIndex;
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
            m_modelData->materials[i].sphereTextureIndex;
        if (sphereTextureIndex >= 0 &&
            m_modelData->materials[i].sphereMode != 0 &&
            m_textures[sphereTextureIndex].id() != 0)
        {
            m_textures[sphereTextureIndex].bind(1);
            m_shader.setUniform1i("u_mat.sphereTextureMode",
                                  m_modelData->materials[i].sphereMode);
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

        int32_t toonTextureIndex = m_modelData->materials[i].toonTextureIndex;
        if (toonTextureIndex >= 0 &&
            (m_modelData->materials[i].sharedToonFlag ||
             m_textures[toonTextureIndex].id() != 0))
        {
            (m_modelData->materials[i].sharedToonFlag
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

        m_shader.setUniform1i("u_receiveShadow",
                              m_modelData->materials[i].receiveShadow());

        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

void ModelRenderer::renderEdge(const Camera &camera) const
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
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

        if (!m_modelData->materials[i].renderEdge() || mat.edgeSize == 0.f ||
            mat.edgeColor.a == 0.f)
            continue;

        m_edgeShader.setUniform1f("u_edgeSize", mat.edgeSize);
        m_edgeShader.setUniform4fv("u_edgeColor", &mat.edgeColor[0]);
        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

void ModelRenderer::renderGroundShadow(const Camera   &camera,
                                       const Lighting &lighting) const
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 proj  = camera.proj();
    glm::mat4 view  = camera.view();
    glm::mat4 MV    = view * model;
    glm::mat4 MVP   = proj * MV;

    m_groundShadowShader.use();
    m_groundShadowShader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
    m_groundShadowShader.setUniform3fv("u_lightDir", &lighting.direction[0]);

    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        if (!m_modelData->materials[i].groundShadow() ||
            m_renderData.materials[i].diffuse.a == 0.f)
            continue;

        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

void ModelRenderer::renderShadowMap(const Lighting &lighting) const
{
    m_VBO.bind();
    m_VAO.bind();

    glEnable(GL_DEPTH_TEST);

    m_shadowMapShader.use();
    m_shadowMapShader.setUniformMatrix4fv(
        "u_lightVP", &(lighting.proj() * lighting.view())[0][0]);
    glm::mat4 model = glm::mat4(1.f);
    m_shadowMapShader.setUniformMatrix4fv("u_model", &model[0][0]);

    for (size_t i = 0; i < m_IBOs.size(); ++i)
    {
        const auto &mat = m_modelData->materials[i];

        if (m_renderData.materials[i].diffuse.a == 0.f || !mat.castShadow())
            continue;

        if (mat.doubleSided())
            glDisable(GL_CULL_FACE);
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        m_IBOs[i].bind();
        glDrawElements(GL_TRIANGLES, m_IBOs[i].getCount(), GL_UNSIGNED_INT,
                       nullptr);
    }
}

} // namespace glmmd