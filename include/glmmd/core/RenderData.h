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
    RenderData(const std::shared_ptr<const ModelData> &data);

    void init();

    void applyMaterialFactors();

    glm::vec3 getVertexPosition(size_t index) const;
    glm::vec3 getVertexNormal(size_t index) const;
    glm::vec2 getVertexUV(size_t index) const;
    glm::vec4 getVertexAdditionalUV(size_t index, size_t uvIndex) const;

    void setVertexPosition(size_t index, const glm::vec3 &position);
    void setVertexNormal(size_t index, const glm::vec3 &normal);
    void setVertexUV(size_t index, const glm::vec2 &uv);
    void setVertexAdditionalUV(size_t index, size_t uvIndex,
                               const glm::vec4 &additionalUV);

    size_t stride;

    std::vector<float> vertexBuffer;

    std::vector<MaterialFactors>    materialAdd;
    std::vector<MaterialFactors>    materialMul;
    std::vector<MaterialRenderData> materials;

private:
    std::shared_ptr<const ModelData> m_data;

    std::vector<float> m_initialVertexBuffer;
};

} // namespace glmmd

#endif
