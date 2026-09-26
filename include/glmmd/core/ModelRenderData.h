#ifndef GLMMD_CORE_MODEL_RENDER_DATA_H_
#define GLMMD_CORE_MODEL_RENDER_DATA_H_

#include <glmmd/core/ModelData.h>

#include <vector>

namespace glmmd {

struct MaterialFactors {
  glm::vec4 diffuse;
  glm::vec3 specular;
  float specularPower;
  glm::vec3 ambient;
  glm::vec4 edgeColor;
  float edgeSize;
  glm::vec4 texture;
  glm::vec4 sphereTexture;
  glm::vec4 toonTexture;

  void initAdd();
  void initMul();
};

struct MaterialRenderData {
  MaterialFactors add;
  MaterialFactors mul;

  glm::vec4 diffuse;
  glm::vec3 specular;
  float specularPower;
  glm::vec3 ambient;
  glm::vec4 edgeColor;
  float edgeSize;
};

// Per-instance render state derived from a Pose: a GPU-ready interleaved vertex
// buffer (position, normal, uv, additional uvs) plus resolved material factors.
// Call init() to reset to the bind pose, then Pose::applyToRenderData to skin
// vertices and apply morphs for the current frame.
//
// Holds a non-owning pointer to its ModelData; the referenced ModelData must
// outlive the ModelRenderData.
struct ModelRenderData {
  ModelRenderData() = default;

  ModelRenderData(const ModelData &data);

  void create(const ModelData &data);

  void init();

  void applyMaterialFactors();

  // Hot-path per-vertex accessors: defined inline so they inline into the
  // skinning/morph loops in Pose (a different translation unit). `stride`
  // varies with the model's additionalUVNum, so the buffer stays a flat
  // std::vector<float> laid out as [pos(3) normal(3) uv(2) addUV(4*n)] per
  // vertex.
  glm::vec3 getVertexPosition(size_t index) const {
    size_t offset = index * stride;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
  }
  glm::vec3 getVertexNormal(size_t index) const {
    size_t offset = index * stride + 3;
    return glm::vec3(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2]);
  }
  glm::vec2 getVertexUV(size_t index) const {
    size_t offset = index * stride + 6;
    return glm::vec2(vertexBuffer[offset], vertexBuffer[offset + 1]);
  }
  glm::vec4 getVertexAdditionalUV(size_t index, size_t uvIndex) const {
    size_t offset = index * stride + 8 + 4 * uvIndex;
    return glm::vec4(vertexBuffer[offset], vertexBuffer[offset + 1],
                     vertexBuffer[offset + 2], vertexBuffer[offset + 3]);
  }

  void setVertexPosition(size_t index, const glm::vec3 &position) {
    size_t offset = index * stride;
    vertexBuffer[offset] = position.x;
    vertexBuffer[offset + 1] = position.y;
    vertexBuffer[offset + 2] = position.z;
  }
  void setVertexNormal(size_t index, const glm::vec3 &normal) {
    size_t offset = index * stride + 3;
    vertexBuffer[offset] = normal.x;
    vertexBuffer[offset + 1] = normal.y;
    vertexBuffer[offset + 2] = normal.z;
  }
  void setVertexUV(size_t index, const glm::vec2 &uv) {
    size_t offset = index * stride + 6;
    vertexBuffer[offset] = uv.x;
    vertexBuffer[offset + 1] = uv.y;
  }
  void setVertexAdditionalUV(size_t index, size_t uvIndex,
                             const glm::vec4 &additionalUV) {
    size_t offset = index * stride + 8 + 4 * uvIndex;
    vertexBuffer[offset] = additionalUV.x;
    vertexBuffer[offset + 1] = additionalUV.y;
    vertexBuffer[offset + 2] = additionalUV.z;
    vertexBuffer[offset + 3] = additionalUV.w;
  }

  size_t stride;

  std::vector<float> vertexBuffer;

  std::vector<MaterialRenderData> materials;

private:
  const ModelData *m_data = nullptr;

  std::vector<float> m_initialVertexBuffer;
};

} // namespace glmmd

#endif
