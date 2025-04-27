#include <algorithm>
#ifndef GLMMD_DONT_PARALLELIZE
#include <execution>
#endif

#include <glmmd/core/ModelRenderData.h>

namespace glmmd
{

void MaterialFactors::initAdd()
{
    diffuse       = glm::vec4(0.f);
    specular      = glm::vec3(0.f);
    specularPower = 0.f;
    ambient       = glm::vec3(0.f);
    edgeColor     = glm::vec4(0.f);
    edgeSize      = 0.f;
    texture       = glm::vec4(0.f);
    sphereTexture = glm::vec4(0.f);
    toonTexture   = glm::vec4(0.f);
}

void MaterialFactors::initMul()
{
    diffuse       = glm::vec4(1.f);
    specular      = glm::vec3(1.f);
    specularPower = 1.f;
    ambient       = glm::vec3(1.f);
    edgeColor     = glm::vec4(1.f);
    edgeSize      = 1.f;
    texture       = glm::vec4(1.f);
    sphereTexture = glm::vec4(1.f);
    toonTexture   = glm::vec4(1.f);
}

ModelRenderData::ModelRenderData(const std::shared_ptr<const ModelData> &data)
{
    create(data);
}

void ModelRenderData::create(const std::shared_ptr<const ModelData> &data)
{
    if (!data)
        return;

    m_data = data;

    stride = 3 + 3 + 2 + 4 * data->info.additionalUVNum;
    vertexBuffer.resize(data->vertices.size() * stride);
    materials.resize(data->materials.size());
    m_initialVertexBuffer.resize(data->vertices.size() * stride);

    for (size_t i = 0; i < m_data->vertices.size(); ++i)
    {
        const auto &vert = m_data->vertices[i];

        size_t offset = i * stride;

        m_initialVertexBuffer[offset++] = vert.position.x;
        m_initialVertexBuffer[offset++] = vert.position.y;
        m_initialVertexBuffer[offset++] = vert.position.z;
        m_initialVertexBuffer[offset++] = vert.normal.x;
        m_initialVertexBuffer[offset++] = vert.normal.y;
        m_initialVertexBuffer[offset++] = vert.normal.z;
        m_initialVertexBuffer[offset++] = vert.uv.x;
        m_initialVertexBuffer[offset++] = vert.uv.y;
        for (uint8_t j = 0; j < data->info.additionalUVNum; ++j)
            for (uint8_t k = 0; k < 4; ++k)
                m_initialVertexBuffer[offset++] = data->additionalUVs[i][j][k];
    }
}

void ModelRenderData::init()
{
    std::copy(
#ifndef GLMMD_DONT_PARALLELIZE
        std::execution::par,
#endif
        m_initialVertexBuffer.begin(), m_initialVertexBuffer.end(),
        vertexBuffer.begin());

    for (size_t i = 0; i < m_data->materials.size(); ++i)
    {
        auto       &mat     = materials[i];
        const auto &dataMat = m_data->materials[i];

        mat.add.initAdd();
        mat.mul.initMul();

        mat.diffuse       = dataMat.diffuse;
        mat.specular      = dataMat.specular;
        mat.specularPower = dataMat.specularPower;
        mat.ambient       = dataMat.ambient;
        mat.edgeColor     = dataMat.edgeColor;
        mat.edgeSize      = dataMat.edgeSize;
    }
}

void ModelRenderData::applyMaterialFactors()
{
    for (size_t i = 0; i < m_data->materials.size(); ++i)
    {
        auto       &mat     = materials[i];
        const auto &dataMat = m_data->materials[i];

        mat.diffuse  = mat.add.diffuse + mat.mul.diffuse * dataMat.diffuse;
        mat.specular = mat.add.specular + mat.mul.specular * dataMat.specular;
        mat.specularPower = mat.add.specularPower +
                            mat.mul.specularPower * dataMat.specularPower;
        mat.ambient = mat.add.ambient + mat.mul.ambient * dataMat.ambient;
        mat.edgeColor =
            mat.add.edgeColor + mat.mul.edgeColor * dataMat.edgeColor;
        mat.edgeSize = mat.add.edgeSize + mat.mul.edgeSize * dataMat.edgeSize;
    }
}

glm::vec3 ModelRenderData::getVertexPosition(size_t index) const
{
    size_t offset = index * stride;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
}

glm::vec3 ModelRenderData::getVertexNormal(size_t index) const
{
    size_t offset = index * stride + 3;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
}

glm::vec2 ModelRenderData::getVertexUV(size_t index) const
{
    size_t offset = index * stride + 6;
    return glm::vec2(vertexBuffer[offset], vertexBuffer[offset + 1]);
}

glm::vec4 ModelRenderData::getVertexAdditionalUV(size_t index,
                                                 size_t uvIndex) const
{
    size_t offset = index * stride + 8 + 4 * uvIndex;
    return glm::vec4(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2], vertexBuffer[offset + 3]);
}

void ModelRenderData::setVertexPosition(size_t index, const glm::vec3 &position)
{
    size_t offset            = index * stride;
    vertexBuffer[offset]     = position.x;
    vertexBuffer[offset + 1] = position.y;
    vertexBuffer[offset + 2] = position.z;
}

void ModelRenderData::setVertexNormal(size_t index, const glm::vec3 &normal)
{
    size_t offset            = index * stride + 3;
    vertexBuffer[offset]     = normal.x;
    vertexBuffer[offset + 1] = normal.y;
    vertexBuffer[offset + 2] = normal.z;
}

void ModelRenderData::setVertexUV(size_t index, const glm::vec2 &uv)
{
    size_t offset            = index * stride + 6;
    vertexBuffer[offset]     = uv.x;
    vertexBuffer[offset + 1] = uv.y;
}

void ModelRenderData::setVertexAdditionalUV(size_t index, size_t uvIndex,
                                            const glm::vec4 &additionalUV)
{
    size_t offset            = index * stride + 8 + 4 * uvIndex;
    vertexBuffer[offset]     = additionalUV.x;
    vertexBuffer[offset + 1] = additionalUV.y;
    vertexBuffer[offset + 2] = additionalUV.z;
    vertexBuffer[offset + 3] = additionalUV.w;
}

} // namespace glmmd
