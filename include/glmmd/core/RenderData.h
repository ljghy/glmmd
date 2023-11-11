#ifndef GLMMD_CORE_RENDER_DATA_H_
#define GLMMD_CORE_RENDER_DATA_H_

#include <memory>
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
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    RenderData(const std::shared_ptr<const ModelData> &data);

    void init();

    void applyMaterialFactors();

    std::vector<Vertex> vertexBuffer;

    std::vector<MaterialFactors>    materialAdd;
    std::vector<MaterialFactors>    materialMul;
    std::vector<MaterialRenderData> materials;

private:
    std::shared_ptr<const ModelData> m_data;

    std::vector<Vertex> m_initialVertexBuffer;
};

} // namespace glmmd

#endif
