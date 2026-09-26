#include <glmmd/core/Pose.h>

#include <glmmd/core/ParallelForEach.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace glmmd {

Pose::Pose(const ModelData &modelData) { create(modelData); }

void Pose::create(const ModelData &modelData) {
  m_modelData = &modelData;
  m_localBoneTransforms.resize(modelData.bones.size(), Transform::identity);
  m_morphWeights.resize(modelData.morphs.size(), 0.f);
  m_globalBoneTransforms.resize(modelData.bones.size(), Transform::identity);
  m_finalBoneTransforms.resize(modelData.bones.size());
}

const Transform &Pose::globalBoneTransform(uint32_t boneIndex) const {
  return m_globalBoneTransforms[boneIndex];
}

glm::vec3 Pose::globalBonePosition(uint32_t boneIndex) const {
  return m_globalBoneTransforms[boneIndex].translation;
}

const Transform &Pose::localBoneTransform(uint32_t boneIndex) const {
  return m_localBoneTransforms[boneIndex];
}

Transform &Pose::localBoneTransform(uint32_t boneIndex) {
  return m_localBoneTransforms[boneIndex];
}

glm::dualquat Pose::finalBoneTransform(uint32_t boneIndex) const {
  auto &r = m_globalBoneTransforms[boneIndex].rotation;
  auto &t = m_globalBoneTransforms[boneIndex].translation;
  return glm::dualquat(r, t - r * m_modelData->bones[boneIndex].position);
}

const glm::vec3 &Pose::localBoneTranslation(uint32_t boneIndex) const {
  return m_localBoneTransforms[boneIndex].translation;
}

glm::vec3 &Pose::localBoneTranslation(uint32_t boneIndex) {
  return m_localBoneTransforms[boneIndex].translation;
}

const glm::quat &Pose::localBoneRotation(uint32_t boneIndex) const {
  return m_localBoneTransforms[boneIndex].rotation;
}

glm::quat &Pose::localBoneRotation(uint32_t boneIndex) {
  return m_localBoneTransforms[boneIndex].rotation;
}

float Pose::morphWeight(uint32_t morphIndex) const {
  return m_morphWeights[morphIndex];
}

float &Pose::morphWeight(uint32_t morphIndex) {
  return m_morphWeights[morphIndex];
}

void Pose::resetLocal() {
  std::fill(m_localBoneTransforms.begin(), m_localBoneTransforms.end(),
            Transform::identity);
  std::fill(m_morphWeights.begin(), m_morphWeights.end(), 0.f);
}

void Pose::applyToRenderData(ModelRenderData &renderData) const {
  applyMorphsToRenderData(renderData);
  applyBoneTransformsToRenderData(renderData);
}

void Pose::applyMorphsToRenderData(ModelRenderData &renderData) const {
  for (size_t i = 0; i < m_morphWeights.size(); ++i) {
    const auto &morph = m_modelData->morphs[i];
    float weight = m_morphWeights[i];
    if (weight == 0.f)
      continue;

    switch (morph.type) {
    case MorphType::Vertex:
      for (const auto &data : morph.vertex()) {
        renderData.setVertexPosition(data.index,
                                     renderData.getVertexPosition(data.index) +
                                         weight * data.offset);
      }
      break;
    case MorphType::Material:
      for (const auto &data : morph.material()) {
        size_t first = 0, last = renderData.materials.size();
        if (data.index >= 0) {
          first = data.index;
          last = first + 1;
        }
        if (data.operation == MaterialMorphOperation::Multiply) // multiply
        {
          for (; first != last; ++first) {
            auto &mul = renderData.materials[first].mul;
            mul.diffuse *= glm::mix(glm::vec4(1.f), data.diffuse, weight);
            mul.specular *= glm::mix(glm::vec3(1.f), data.specular, weight);
            mul.specularPower *= glm::mix(1.f, data.specularPower, weight);
            mul.ambient *= glm::mix(glm::vec3(1.f), data.ambient, weight);
            mul.edgeColor *= glm::mix(glm::vec4(1.f), data.edgeColor, weight);
            mul.edgeSize *= glm::mix(1.f, data.edgeSize, weight);
            mul.texture *= glm::mix(glm::vec4(1.f), data.texture, weight);
            mul.sphereTexture *=
                glm::mix(glm::vec4(1.f), data.sphereTexture, weight);
            mul.toonTexture *=
                glm::mix(glm::vec4(1.f), data.toonTexture, weight);
          }
        } else // add
        {
          for (; first != last; ++first) {
            auto &add = renderData.materials[first].add;
            add.diffuse += weight * data.diffuse;
            add.specular += weight * data.specular;
            add.specularPower += weight * data.specularPower;
            add.ambient += weight * data.ambient;
            add.edgeColor += weight * data.edgeColor;
            add.edgeSize += weight * data.edgeSize;
            add.texture += weight * data.texture;
            add.sphereTexture += weight * data.sphereTexture;
            add.toonTexture += weight * data.toonTexture;
          }
        }
      }
      break;
    case MorphType::UV:
      for (const auto &data : morph.uv()) {
        renderData.setVertexUV(data.index,
                               renderData.getVertexUV(data.index) +
                                   weight * glm::vec2(data.offset[0]));
      }
      break;
    case MorphType::UV1:
    case MorphType::UV2:
    case MorphType::UV3:
    case MorphType::UV4: {
      auto uvIndex = static_cast<uint8_t>(morph.type) -
                     static_cast<uint8_t>(MorphType::UV1);
      for (const auto &data : morph.uv()) {
        renderData.setVertexAdditionalUV(
            data.index, uvIndex,
            renderData.getVertexAdditionalUV(data.index, uvIndex) +
                weight * data.offset[1 + uvIndex]);
      }
    } break;
    default:
      break;
    }
  }

  renderData.applyMaterialFactors();
}

void Pose::applyBoneTransformsToRenderData(ModelRenderData &renderData) const {
  auto &finalBoneTransforms = m_finalBoneTransforms;
  for (uint32_t i = 0; i < finalBoneTransforms.size(); ++i)
    finalBoneTransforms[i] = finalBoneTransform(i);

  parallelForEach(
      m_modelData->vertices.begin(), m_modelData->vertices.end(),
      [&](const Vertex &vert) {
        auto i = static_cast<uint32_t>(&vert - &m_modelData->vertices[0]);
        auto pos = renderData.getVertexPosition(i);
        auto norm = renderData.getVertexNormal(i);

        if (vert.skinningType == SkinningType::SDEF) {
          const auto &dq0 = finalBoneTransforms[vert.boneIndices[0]];
          const auto &dq1 = finalBoneTransforms[vert.boneIndices[1]];
          const auto &q0 = dq0.real;
          const auto &q1 = dq1.real;

          float w0 = vert.boneWeights[0];
          float w1 = 1.f - w0;

          auto q = glm::slerp(q0, q1, w1);

          const auto &c = vert.sdefC;

          auto r = 0.5f * (vert.sdefR0 - vert.sdefR1);

          pos = q * (pos - c) + (dq0 * (c + w1 * r)) * w0 +
                (dq1 * (c - w0 * r)) * w1;
          norm = q * norm;
        } else if (vert.skinningType == SkinningType::QDEF) {
          glm::dualquat dq = finalBoneTransforms[vert.boneIndices[0]];
          auto q0 = dq.real;
          dq *= vert.boneWeights[0];
          for (int bi = 1; bi < 4; ++bi) {
            float w = vert.boneWeights[bi];
            if (glm::dot(q0, finalBoneTransforms[vert.boneIndices[bi]].real) <
                0)
              w = -w;
            dq = dq + w * finalBoneTransforms[vert.boneIndices[bi]];
          }
          dq = glm::normalize(dq);

          pos = dq * pos;
          norm = dq.real * norm;
        } else {
          const int nb =
              vert.skinningType == SkinningType::BDEF1
                  ? 1
                  : (vert.skinningType == SkinningType::BDEF2 ? 2 : 4);
          glm::vec3 skinnedPos(0.f);
          glm::vec3 skinnedNorm(0.f);
          for (int bi = 0; bi < nb; ++bi) {
            const float weight = nb == 1 ? 1.f : vert.boneWeights[bi];
            const auto &dq = finalBoneTransforms[vert.boneIndices[bi]];
            skinnedPos += weight * (dq * pos);
            skinnedNorm += weight * (dq.real * norm);
          }
          pos = skinnedPos;
          norm = skinnedNorm;
        }

        renderData.setVertexPosition(i, pos);
        renderData.setVertexNormal(i, norm);
      });
}

void Pose::blendWith(const Pose &other, float t) {
  for (size_t i = 0; i < m_localBoneTransforms.size(); ++i)
    m_localBoneTransforms[i] = ((1.f - t) * m_localBoneTransforms[i]) *
                               (t * other.m_localBoneTransforms[i]);

  for (size_t i = 0; i < m_morphWeights.size(); ++i)
    m_morphWeights[i] = glm::mix(m_morphWeights[i], other.m_morphWeights[i], t);
}

void Pose::compose(const Pose &other) {
  for (size_t i = 0; i < m_localBoneTransforms.size(); ++i)
    m_localBoneTransforms[i] =
        m_localBoneTransforms[i] * other.m_localBoneTransforms[i];

  for (size_t i = 0; i < m_morphWeights.size(); ++i)
    m_morphWeights[i] = other.m_morphWeights[i] + m_morphWeights[i];
}

void Pose::scale(float t) {
  for (auto &localBoneTransform : m_localBoneTransforms)
    localBoneTransform *= t;

  for (auto &morphWeight : m_morphWeights)
    morphWeight *= t;
}

} // namespace glmmd
