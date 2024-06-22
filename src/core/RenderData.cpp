#include <algorithm>
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
#include <execution>
#endif

#include <glmmd/core/RenderData.h>

namespace glmmd
{

RenderData::RenderData(const std::shared_ptr<const ModelData> &data)
    : stride(3 + 3 + 2 + 4 * data->info.additionalUVNum)
    , vertexBuffer(data->vertices.size() * stride)
    , materialAdd(data->materials.size())
    , materialMul(data->materials.size())
    , materials(data->materials.size())
    , m_data(data)
    , m_initialVertexBuffer(data->vertices.size() * stride)
{
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
                m_initialVertexBuffer[offset++] = vert.additionalUVs[j][k];
    }
}

void RenderData::init()
{
    std::copy(
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
        std::execution::par,
#endif
        m_initialVertexBuffer.begin(), m_initialVertexBuffer.end(),
        vertexBuffer.begin());

    for (size_t i = 0; i < m_data->materials.size(); ++i)
    {
        materials[i].diffuse       = m_data->materials[i].diffuse;
        materials[i].specular      = m_data->materials[i].specular;
        materials[i].specularPower = m_data->materials[i].specularPower;
        materials[i].ambient       = m_data->materials[i].ambient;
        materials[i].edgeColor     = m_data->materials[i].edgeColor;
        materials[i].edgeSize      = m_data->materials[i].edgeSize;

        materialAdd[i].diffuse       = glm::vec4(0.f);
        materialAdd[i].specular      = glm::vec3(0.f);
        materialAdd[i].specularPower = 0.f;
        materialAdd[i].ambient       = glm::vec3(0.f);
        materialAdd[i].edgeColor     = glm::vec4(0.f);
        materialAdd[i].edgeSize      = 0.f;
        materialAdd[i].texture       = glm::vec4(0.f);
        materialAdd[i].sphereTexture = glm::vec4(0.f);
        materialAdd[i].toonTexture   = glm::vec4(0.f);

        materialMul[i].diffuse       = glm::vec4(1.f);
        materialMul[i].specular      = glm::vec3(1.f);
        materialMul[i].specularPower = 1.f;
        materialMul[i].ambient       = glm::vec3(1.f);
        materialMul[i].edgeColor     = glm::vec4(1.f);
        materialMul[i].edgeSize      = 1.f;
        materialMul[i].texture       = glm::vec4(1.f);
        materialMul[i].sphereTexture = glm::vec4(1.f);
        materialMul[i].toonTexture   = glm::vec4(1.f);
    }
}

void RenderData::applyMaterialFactors()
{
    for (size_t i = 0; i < m_data->materials.size(); ++i)
    {
        materials[i].diffuse =
            materialAdd[i].diffuse +
            materialMul[i].diffuse * m_data->materials[i].diffuse;
        materials[i].specular =
            materialAdd[i].specular +
            materialMul[i].specular * m_data->materials[i].specular;
        materials[i].specularPower =
            materialAdd[i].specularPower +
            materialMul[i].specularPower * m_data->materials[i].specularPower;
        materials[i].ambient =
            materialAdd[i].ambient +
            materialMul[i].ambient * m_data->materials[i].ambient;
        materials[i].edgeColor =
            materialAdd[i].edgeColor +
            materialMul[i].edgeColor * m_data->materials[i].edgeColor;
        materials[i].edgeSize =
            materialAdd[i].edgeSize +
            materialMul[i].edgeSize * m_data->materials[i].edgeSize;
    }
}

glm::vec3 RenderData::getVertexPosition(size_t index) const
{
    size_t offset = index * stride;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
}

glm::vec3 RenderData::getVertexNormal(size_t index) const
{
    size_t offset = index * stride + 3;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
}

glm::vec2 RenderData::getVertexUV(size_t index) const
{
    size_t offset = index * stride + 6;
    return glm::vec2(vertexBuffer[offset], vertexBuffer[offset + 1]);
}

glm::vec4 RenderData::getVertexAdditionalUV(size_t index, size_t uvIndex) const
{
    size_t offset = index * stride + 8 + 4 * uvIndex;
    return glm::vec4(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2], vertexBuffer[offset + 3]);
}

void RenderData::setVertexPosition(size_t index, const glm::vec3 &position)
{
    size_t offset            = index * stride;
    vertexBuffer[offset]     = position.x;
    vertexBuffer[offset + 1] = position.y;
    vertexBuffer[offset + 2] = position.z;
}

void RenderData::setVertexNormal(size_t index, const glm::vec3 &normal)
{
    size_t offset            = index * stride + 3;
    vertexBuffer[offset]     = normal.x;
    vertexBuffer[offset + 1] = normal.y;
    vertexBuffer[offset + 2] = normal.z;
}

void RenderData::setVertexUV(size_t index, const glm::vec2 &uv)
{
    size_t offset            = index * stride + 6;
    vertexBuffer[offset]     = uv.x;
    vertexBuffer[offset + 1] = uv.y;
}

void RenderData::setVertexAdditionalUV(size_t index, size_t uvIndex,
                                       const glm::vec4 &additionalUV)
{
    size_t offset            = index * stride + 8 + 4 * uvIndex;
    vertexBuffer[offset]     = additionalUV.x;
    vertexBuffer[offset + 1] = additionalUV.y;
    vertexBuffer[offset + 2] = additionalUV.z;
    vertexBuffer[offset + 3] = additionalUV.w;
}

} // namespace glmmd
