#include <glmmd/core/RenderData.h>

namespace glmmd
{

RenderData::RenderData(const std::shared_ptr<const ModelData> &data)
    : positions(data->vertices.size())
    , normals(data->vertices.size())
    , UVs(data->vertices.size())
    , materialAdd(data->materials.size())
    , materialMul(data->materials.size())
    , materials(data->materials.size())
    , m_data(data)
    , m_initialPositions(data->vertices.size())
    , m_initialNormals(data->vertices.size())
    , m_initialUVs(data->vertices.size())
{
    for (size_t i = 0; i < m_data->vertices.size(); ++i)
    {
        m_initialPositions[i] = m_data->vertices[i].position;
        m_initialNormals[i]   = m_data->vertices[i].normal;
        m_initialUVs[i]       = m_data->vertices[i].uv;
    }
}

void RenderData::init()
{
    positions = m_initialPositions;
    normals   = m_initialNormals;
    UVs       = m_initialUVs;

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

} // namespace glmmd
