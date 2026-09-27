#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>

#include "MaterialShader.h"
#include "ShadowMap.h"

#include <stb/stb_image.h>

#include "DefaultShaderSources.inl"
#include "ModelRenderer.h"
#include <glmmd/core/SharedToonTextures.h>

std::mutex sharedToonTexturesInitMutex;

bool ModelRenderer::sharedToonTexturesLoaded = false;
std::array<ogl::Texture2D, 10> ModelRenderer::sharedToonTextures;

void ModelRenderer::releaseSharedToonTextures() {
  std::lock_guard<std::mutex> lock(sharedToonTexturesInitMutex);
  if (!sharedToonTexturesLoaded)
    return;
  for (auto &texture : sharedToonTextures)
    texture.destroy();
  sharedToonTexturesLoaded = false;
}

ModelRenderer::ModelRenderer(
    const std::shared_ptr<const glmmd::ModelData> &data,
    const ModelRendererShaderSources &shaderSources)
    : m_modelData(data), m_renderData(*data),
      m_extensions(parseMaterialExtensions(*data)) {
  initBuffers();
  m_textures.resize(m_extensions.texturePaths.size());
  m_dataTextures.resize(m_textures.size());
  initTextures();
  initSharedToonTextures();
  initShaders(shaderSources);
}

void ModelRenderer::initBuffers() {
  m_VBO.create(nullptr,
               static_cast<unsigned int>(sizeof(GLfloat) *
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
  m_IBO.create(
      m_modelData->indices.data(),
      static_cast<unsigned int>(sizeof(GLuint) * m_modelData->indices.size()));
}

void ModelRenderer::initTextures() {
  std::vector<bool> colorUsed(m_textures.size()), dataUsed(m_textures.size());
  auto mark = [](std::vector<bool> &used, int32_t index) {
    if (index >= 0 && static_cast<size_t>(index) < used.size())
      used[index] = true;
  };
  for (size_t i = 0; i < m_modelData->materials.size(); ++i) {
    const auto &material = m_modelData->materials[i];
    const auto &extension = m_extensions.materials[i];
    mark(colorUsed, material.textureIndex);
    mark(colorUsed, material.sphereTextureIndex);
    if (!material.sharedToonFlag)
      mark(colorUsed, material.toonTextureIndex);
    mark(dataUsed, extension.normalTextureIndex);
    mark(dataUsed, extension.occlusionTextureIndex);
  }
  for (size_t i = 0; i < m_extensions.texturePaths.size(); ++i) {
    if (!colorUsed[i] && !dataUsed[i])
      continue;
    const auto &relPath = m_extensions.texturePaths[i];

    std::filesystem::path path =
        m_modelData->baseDir / std::u8string(relPath.begin(), relPath.end());

    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
      std::cout << "Failed to load texture: " << relPath << std::endl;
      continue;
    }
    std::vector<stbi_uc> buffer(std::istreambuf_iterator<char>{fin},
                                std::istreambuf_iterator<char>{});
    fin.close();

    int width, height, channels;
    stbi_uc *data =
        stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()),
                              &width, &height, &channels, 4);

    if (!data) {
      std::cout << "Failed to load texture: " << relPath << std::endl;
      continue;
    }

    ogl::Texture2DCreateInfo info;
    info.width = width;
    info.height = height;
    info.data = data;
    info.genMipmaps = true;
    info.internalFmt = GL_SRGB_ALPHA;
    info.dataFmt = GL_RGBA;
    info.dataType = GL_UNSIGNED_BYTE;
    info.wrapModeS = GL_REPEAT;
    info.wrapModeT = GL_REPEAT;
    info.minFilterMode = GL_LINEAR_MIPMAP_LINEAR;
    info.magFilterMode = GL_LINEAR;

    if (colorUsed[i])
      m_textures[i].create(info);
    if (dataUsed[i]) {
      info.internalFmt = GL_RGBA8;
      m_dataTextures[i].create(info);
    }

    stbi_image_free(data);
  }
}

void ModelRenderer::initSharedToonTextures() {
  std::lock_guard<std::mutex> lock(sharedToonTexturesInitMutex);
  if (!sharedToonTexturesLoaded) {
    for (size_t i = 0; i < sharedToonTextures.size(); ++i) {
      ogl::Texture2DCreateInfo info;
      info.width = 32;
      info.height = 32;
      info.data = const_cast<uint8_t *>(glmmd::sharedToonTextureData[i]);
      info.genMipmaps = false;
      info.internalFmt = GL_SRGB;
      info.dataFmt = GL_RGB;
      info.wrapModeS = GL_CLAMP_TO_EDGE;
      info.wrapModeT = GL_CLAMP_TO_EDGE;
      sharedToonTextures[i].create(info);
    }
    sharedToonTexturesLoaded = true;
  }
}

void ModelRenderer::initShaders(
    const ModelRendererShaderSources &shaderSources) {
  std::string vertShaderSrc = shaderSources.vertShaderSrc;
  std::string fragShaderSrc = shaderSources.fragShaderSrc;

  if (m_modelData->info.additionalUVNum > 0) {
    std::string vertShaderAdditionalUVLayout;
    std::string vertShaderAdditionalUVOut;
    std::string vertShaderAdditionalUVV2F;
    std::string fragShaderAdditionalUVIn = "#define USE_ADDITIONAL_UV\n";
    for (uint8_t i = 0; i < m_modelData->info.additionalUVNum; ++i) {
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

  m_shader.create(
      vertShaderSrc.c_str(),
      surfaceFragmentShader(materialFragmentShader(fragShaderSrc)).c_str());
  m_edgeShader.create(
      shaderSources.edgeVertShaderSrc,
      surfaceFragmentShader(shaderSources.edgeFragShaderSrc).c_str());
  m_shadowMapShader.create(
      shaderSources.shadowMapVertShaderSrc,
      materialFragmentShader(shaderSources.shadowMapFragShaderSrc).c_str());
  m_groundShadowShader.create(
      shaderSources.groundShadowVertShaderSrc,
      materialFragmentShader(shaderSources.groundShadowFragShaderSrc).c_str());
}

void ModelRenderer::fillBuffers() const {
  m_VBO.bind();
  m_VAO.bind();
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(GLfloat) * m_renderData.vertexBuffer.size(),
               m_renderData.vertexBuffer.data(), GL_DYNAMIC_DRAW);
}

void ModelRenderer::render(SurfacePass pass, const glmmd::Camera &camera,
                           const glmmd::DirectionalLight &light,
                           const ShadowMap *shadowMap) const {
  if (m_renderFlag & MODEL_RENDER_FLAG_HIDE)
    return;

  m_VBO.bind();
  m_VAO.bind();
  m_IBO.bind();

  if (m_renderFlag & MODEL_RENDER_FLAG_MESH)
    renderMesh(pass, camera, light, shadowMap);

  if (m_renderFlag & MODEL_RENDER_FLAG_EDGE)
    renderEdge(pass, camera);

  if (pass == SurfacePass::Opaque &&
      (m_renderFlag & MODEL_RENDER_FLAG_GROUND_SHADOW))
    renderGroundShadow(camera, light);
}

void ModelRenderer::renderMesh(SurfacePass pass, const glmmd::Camera &camera,
                               const glmmd::DirectionalLight &light,
                               const ShadowMap *shadowMap) const {
  glDepthFunc(GL_LEQUAL);
  glCullFace(GL_BACK);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glm::mat4 model = glm::mat4(1.f);
  glm::mat4 proj = camera.proj();
  glm::mat4 view = camera.view();
  glm::mat4 MV = view * model;
  glm::mat4 MVP = proj * MV;

  glm::vec3 dir = camera.front();

  m_shader.use();
  setSurfacePass(m_shader, pass, camera);
  m_shader.setUniformMatrix4fv("u_model", &model[0][0]);
  m_shader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
  m_shader.setUniformMatrix4fv("u_view", &view[0][0]);

  m_shader.setUniform3fv("u_viewDir", &dir[0]);
  m_shader.setUniform3fv("u_lightDir", &light.direction[0]);
  m_shader.setUniform3fv("u_lightColor", &light.color[0]);
  m_shader.setUniform3fv("u_ambientColor", &light.ambientColor[0]);

  m_shader.setUniformMatrix4fv("u_lightVP",
                               &(light.proj() * light.view())[0][0]);

  m_shader.setUniform1i("u_hasShadowMap", shadowMap != nullptr);
  // Keep shadow and ordinary samplers on distinct units even without a map.
  m_shader.setUniform1i("u_shadowMap", 3);
  if (shadowMap)
    shadowMap->bindReceiver(m_shader, camera);

  for (size_t i = 0, indexOffset = 0; i < m_modelData->materials.size();
       indexOffset += m_modelData->materials[i++].indicesCount) {
    const auto &mat = m_renderData.materials[i];
    const auto &matAdd = m_renderData.materials[i].add;
    const auto &matMul = m_renderData.materials[i].mul;

    const auto &extension = m_extensions.materials[i];
    if (mat.diffuse.a <= 0.f &&
        extension.alphaMode != MaterialAlphaMode::Opaque)
      continue;
    m_shader.setUniform1i("u_alphaMode", static_cast<int>(extension.alphaMode));
    const auto bindDataTexture = [&](int32_t index, int unit,
                                     const char *hasName,
                                     const char *samplerName) {
      const bool present = index >= 0 &&
                           static_cast<size_t>(index) < m_dataTextures.size() &&
                           m_dataTextures[index].id() != 0;
      m_shader.setUniform1i(hasName, present);
      m_shader.setUniform1i(samplerName, unit);
      if (present)
        m_dataTextures[index].bind(unit);
    };
    bindDataTexture(extension.normalTextureIndex, 4, "u_hasNormalTexture",
                    "u_normalTexture");
    bindDataTexture(extension.occlusionTextureIndex, 5, "u_hasOcclusionTexture",
                    "u_occlusionTexture");

    m_modelData->materials[i].doubleSided() ? glDisable(GL_CULL_FACE)
                                            : glEnable(GL_CULL_FACE);

    m_shader.setUniform4fv("u_mat.diffuse", &mat.diffuse[0]);
    m_shader.setUniform3fv("u_mat.specular", &mat.specular[0]);
    m_shader.setUniform1f("u_mat.specularPower", mat.specularPower);
    m_shader.setUniform3fv("u_mat.ambient", &mat.ambient[0]);

    int32_t textureIndex = m_modelData->materials[i].textureIndex;
    if (textureIndex >= 0 && m_textures[textureIndex].id() != 0) {
      m_textures[textureIndex].bind(0);
      m_shader.setUniform1i("u_mat.hasTexture", 1);
      m_shader.setUniform4fv("u_mat.textureAdd", &matAdd.texture[0]);
      m_shader.setUniform4fv("u_mat.textureMul", &matMul.texture[0]);
      m_shader.setUniform1i("u_mat.texture", 0);
    } else {
      m_shader.setUniform1i("u_mat.hasTexture", 0);
    }

    int32_t sphereTextureIndex = m_modelData->materials[i].sphereTextureIndex;
    if (sphereTextureIndex >= 0 &&
        m_modelData->materials[i].sphereMode != glmmd::SphereMode::None &&
        m_textures[sphereTextureIndex].id() != 0) {
      m_textures[sphereTextureIndex].bind(1);
      if (m_modelData->materials[i].sphereMode == glmmd::SphereMode::Multiply ||
          m_modelData->materials[i].sphereMode == glmmd::SphereMode::Add)
        m_clampSampler.bind(1);
      else
        glBindSampler(1,
                      0); // UV-based subtextures use the texture's repeat mode.
      m_shader.setUniform1i(
          "u_mat.sphereTextureMode",
          static_cast<int>(m_modelData->materials[i].sphereMode));
      m_shader.setUniform4fv("u_mat.sphereTextureAdd",
                             &matAdd.sphereTexture[0]);
      m_shader.setUniform4fv("u_mat.sphereTextureMul",
                             &matMul.sphereTexture[0]);
      m_shader.setUniform1i("u_mat.sphereTexture", 1);
    } else {
      m_shader.setUniform1i("u_mat.sphereTextureMode", 0);
    }

    int32_t toonTextureIndex = m_modelData->materials[i].toonTextureIndex;
    if (toonTextureIndex >= 0 && (m_modelData->materials[i].sharedToonFlag ||
                                  m_textures[toonTextureIndex].id() != 0)) {
      (m_modelData->materials[i].sharedToonFlag
           ? sharedToonTextures[toonTextureIndex]
           : m_textures[toonTextureIndex])
          .bind(2);

      // Shared toon textures have no mipmaps. External ones can use them.
      (m_modelData->materials[i].sharedToonFlag ? m_toonSampler
                                                : m_clampSampler)
          .bind(2);

      m_shader.setUniform1i("u_mat.hasToonTexture", 1);
      m_shader.setUniform4fv("u_mat.toonTextureAdd", &matAdd.toonTexture[0]);
      m_shader.setUniform4fv("u_mat.toonTextureMul", &matMul.toonTexture[0]);
      m_shader.setUniform1i("u_mat.toonTexture", 2);
    } else {
      m_shader.setUniform1i("u_mat.hasToonTexture", 0);
    }

    m_shader.setUniform1i("u_receiveShadow",
                          m_modelData->materials[i].receiveShadow());

    glDrawElements(GL_TRIANGLES, m_modelData->materials[i].indicesCount,
                   GL_UNSIGNED_INT,
                   (const void *)(uintptr_t)(indexOffset * sizeof(GLuint)));
  }
  glBindSampler(1, 0);
  glBindSampler(2, 0);
}

void ModelRenderer::renderEdge(SurfacePass pass,
                               const glmmd::Camera &camera) const {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(2.f, 2.f);

  glm::mat4 model = glm::mat4(1.f);
  glm::mat4 proj = camera.proj();
  glm::mat4 view = camera.view();
  glm::mat4 MV = view * model;
  glm::mat4 MVP = proj * MV;

  m_edgeShader.use();
  setSurfacePass(m_edgeShader, pass, camera);
  m_edgeShader.setUniformMatrix4fv("u_MV", &MV[0][0]);
  m_edgeShader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);
  m_edgeShader.setUniform2fv(
      "u_viewportSize",
      &glm::vec2(camera.viewportWidth, camera.viewportHeight)[0]);

  for (size_t i = 0, indexOffset = 0; i < m_modelData->materials.size();
       indexOffset += m_modelData->materials[i++].indicesCount) {
    const auto &mat = m_renderData.materials[i];

    if (!m_modelData->materials[i].renderEdge() || mat.edgeSize == 0.f ||
        mat.edgeColor.a == 0.f)
      continue;

    m_edgeShader.setUniform1f("u_edgeSize", mat.edgeSize);
    m_edgeShader.setUniform4fv("u_edgeColor", &mat.edgeColor[0]);

    glDrawElements(GL_TRIANGLES, m_modelData->materials[i].indicesCount,
                   GL_UNSIGNED_INT,
                   (const void *)(uintptr_t)(indexOffset * sizeof(GLuint)));
  }

  glPolygonOffset(0.f, 0.f);
  glDisable(GL_POLYGON_OFFSET_FILL);
}

void ModelRenderer::renderGroundShadow(
    const glmmd::Camera &camera, const glmmd::DirectionalLight &light) const {
  if (light.direction.y == 0.f)
    return;

  glDepthFunc(GL_LESS);
  glDisable(GL_CULL_FACE);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glm::vec3 d = light.direction / light.direction.y;
  glm::mat4 model = glm::mat4(1.f);

  const float offset = 1e-2f;

  model[1][0] = -d.x;
  model[1][1] = 0.f;
  model[1][2] = -d.z;
  model[3][0] = d.x * offset;
  model[3][1] = offset;
  model[3][2] = d.z * offset;

  glm::mat4 MVP = camera.proj() * camera.view() * model;

  m_groundShadowShader.use();
  m_groundShadowShader.setUniform1i("u_texture", 0);
  m_groundShadowShader.setUniformMatrix4fv("u_MVP", &MVP[0][0]);

  for (size_t i = 0, indexOffset = 0; i < m_modelData->materials.size();
       indexOffset += m_modelData->materials[i++].indicesCount) {
    const auto &material = m_modelData->materials[i];
    const float alpha = m_renderData.materials[i].diffuse.a;
    if (!material.groundShadow() ||
        (alpha <= 0.f &&
         m_extensions.materials[i].alphaMode != MaterialAlphaMode::Opaque))
      continue;

    m_groundShadowShader.setUniform1i(
        "u_alphaMode", static_cast<int>(m_extensions.materials[i].alphaMode));
    m_groundShadowShader.setUniform1f("u_diffuseAlpha", alpha);
    const bool hasTexture = material.textureIndex >= 0 &&
                            m_textures[material.textureIndex].id() != 0;
    m_groundShadowShader.setUniform1i("u_hasTexture", hasTexture);
    if (hasTexture)
      m_textures[material.textureIndex].bind(0);

    glDrawElements(GL_TRIANGLES, m_modelData->materials[i].indicesCount,
                   GL_UNSIGNED_INT,
                   (const void *)(uintptr_t)(indexOffset * sizeof(GLuint)));
  }
}

void ModelRenderer::renderShadowMap(const glmmd::DirectionalLight &light,
                                    float alphaCutoff) const {
  if (m_renderFlag & MODEL_RENDER_FLAG_HIDE)
    return;

  m_VBO.bind();
  m_VAO.bind();
  m_IBO.bind();

  glEnable(GL_DEPTH_TEST);

  m_shadowMapShader.use();
  m_shadowMapShader.setUniform1f("u_alphaCutoff", alphaCutoff);
  m_shadowMapShader.setUniform1i("u_texture", 0);
  m_shadowMapShader.setUniformMatrix4fv("u_lightVP",
                                        &(light.proj() * light.view())[0][0]);
  glm::mat4 model = glm::mat4(1.f);
  m_shadowMapShader.setUniformMatrix4fv("u_model", &model[0][0]);

  for (size_t i = 0, indexOffset = 0; i < m_modelData->materials.size();
       indexOffset += m_modelData->materials[i++].indicesCount) {
    const auto &mat = m_modelData->materials[i];

    if (!mat.castShadow() ||
        (m_renderData.materials[i].diffuse.a <= 0.f &&
         m_extensions.materials[i].alphaMode != MaterialAlphaMode::Opaque))
      continue;

    m_shadowMapShader.setUniform1i(
        "u_alphaMode", static_cast<int>(m_extensions.materials[i].alphaMode));
    m_shadowMapShader.setUniform1f("u_diffuseAlpha",
                                   m_renderData.materials[i].diffuse.a);
    const bool hasTexture =
        mat.textureIndex >= 0 && m_textures[mat.textureIndex].id() != 0;
    m_shadowMapShader.setUniform1i("u_hasTexture", hasTexture);
    if (hasTexture)
      m_textures[mat.textureIndex].bind(0);

    if (mat.doubleSided())
      glDisable(GL_CULL_FACE);
    else {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
    }

    glDrawElements(GL_TRIANGLES, m_modelData->materials[i].indicesCount,
                   GL_UNSIGNED_INT,
                   (const void *)(uintptr_t)(indexOffset * sizeof(GLuint)));
  }
}

bool ModelRenderer::shadowBounds(const glm::mat4 &orientation, glm::vec3 &lower,
                                 glm::vec3 &upper) const {
  if (m_renderFlag & MODEL_RENDER_FLAG_HIDE)
    return false;
  lower = glm::vec3(std::numeric_limits<float>::max());
  upper = glm::vec3(std::numeric_limits<float>::lowest());
  bool found = false;
  for (size_t i = 0, offset = 0; i < m_modelData->materials.size();
       offset += m_modelData->materials[i++].indicesCount) {
    const auto &material = m_modelData->materials[i];
    if (!material.castShadow() ||
        (m_renderData.materials[i].diffuse.a <= 0.f &&
         m_extensions.materials[i].alphaMode != MaterialAlphaMode::Opaque))
      continue;
    for (int j = 0; j < material.indicesCount; ++j) {
      const auto index = m_modelData->indices[offset + j];
      const auto position = m_renderData.getVertexPosition(index);
      const auto p = glm::vec3(orientation * glm::vec4(position, 1.f));
      lower = glm::min(lower, p);
      upper = glm::max(upper, p);
      found = true;
    }
  }
  return found;
}
