#ifndef GLMMD_CORE_RENDER_DATA_H_
#define GLMMD_CORE_RENDER_DATA_H_

#include <vector>

#include <glmmd/core/ModelData.h>

namespace glmmd
{

struct MaterialFactors
{
    glm::vec4 diffuse;
    glm::vec3 specular;
    float     specularPower;
    glm::vec3 ambient;
    glm::vec4 edgeColor;
    float     edgeSize;
    glm::vec4 texture;
    glm::vec4 sphereTexture;
    glm::vec4 toonTexture;
};

struct MaterialRenderData
{
    glm::vec4 diffuse;
    glm::vec3 specular;
    float     specularPower;
    glm::vec3 ambient;
    glm::vec4 edgeColor;
    float     edgeSize;
};

struct RenderData
{
    RenderData(const ModelData &data);

    void init();

    void applyMaterialFactors();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> UVs;

    std::vector<MaterialFactors>    materialAdd;
    std::vector<MaterialFactors>    materialMul;
    std::vector<MaterialRenderData> materials;

private:
    const ModelData       &m_data;
    std::vector<glm::vec3> m_initialPositions;
    std::vector<glm::vec3> m_initialNormals;
    std::vector<glm::vec2> m_initialUVs;
};

} // namespace glmmd

#endif
