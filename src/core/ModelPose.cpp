#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>

#include <glmmd/core/ModelPose.h>
#include <glmmd/core/ParallelForEach.h>

namespace glmmd
{

ModelPose::ModelPose(const std::shared_ptr<const ModelData> &modelData)
{
    create(modelData);
}

void ModelPose::create(const std::shared_ptr<const ModelData> &modelData)
{
    if (!modelData)
        return;

    m_modelData = modelData;
    m_localBoneTransforms.resize(modelData->bones.size(), Transform::identity);
    m_morphRatios.resize(modelData->morphs.size(), 0.f);
    m_globalBoneTransforms.resize(modelData->bones.size(), Transform::identity);
}

const Transform &ModelPose::getGlobalBoneTransform(uint32_t boneIndex) const
{
    return m_globalBoneTransforms[boneIndex];
}

glm::vec3 ModelPose::getGlobalBonePosition(uint32_t boneIndex) const
{
    return m_globalBoneTransforms[boneIndex].translation;
}

void ModelPose::setLocalBoneTransform(uint32_t         boneIndex,
                                      const Transform &transform)
{
    m_localBoneTransforms[boneIndex] = transform;
}

void ModelPose::setLocalBoneTranslation(uint32_t         boneIndex,
                                        const glm::vec3 &translation)
{
    m_localBoneTransforms[boneIndex].translation = translation;
}

void ModelPose::setLocalBoneRotation(uint32_t         boneIndex,
                                     const glm::quat &rotation)
{
    m_localBoneTransforms[boneIndex].rotation = rotation;
}

void ModelPose::setMorphRatio(uint32_t morphIndex, float ratio)
{
    m_morphRatios[morphIndex] = ratio;
}

glm::mat4 ModelPose::getFinalBoneTransform(uint32_t boneIndex) const
{
    return glm::translate(m_globalBoneTransforms[boneIndex].toMatrix(),
                          -m_modelData->bones[boneIndex].position);
}

const glm::vec3 &ModelPose::getLocalBoneTranslation(uint32_t boneIndex) const
{
    return m_localBoneTransforms[boneIndex].translation;
}

glm::vec3 &ModelPose::localBoneTranslation(uint32_t boneIndex)
{
    return m_localBoneTransforms[boneIndex].translation;
}

const glm::quat &ModelPose::getLocalBoneRotation(uint32_t boneIndex) const
{
    return m_localBoneTransforms[boneIndex].rotation;
}

glm::quat &ModelPose::localBoneRotation(uint32_t boneIndex)
{
    return m_localBoneTransforms[boneIndex].rotation;
}

float ModelPose::getMorphRatio(uint32_t morphIndex) const
{
    return m_morphRatios[morphIndex];
}

float &ModelPose::morphRatio(uint32_t morphIndex)
{
    return m_morphRatios[morphIndex];
}

void ModelPose::resetLocal()
{
    std::fill(m_localBoneTransforms.begin(), m_localBoneTransforms.end(),
              Transform::identity);
    std::fill(m_morphRatios.begin(), m_morphRatios.end(), 0.f);
}

void ModelPose::applyToRenderData(ModelRenderData &renderData) const
{
    applyMorphsToRenderData(renderData);
    applyBoneTransformsToRenderData(renderData);
}

void ModelPose::applyMorphsToRenderData(ModelRenderData &renderData) const
{
    for (size_t i = 0; i < m_morphRatios.size(); ++i)
    {
        const auto &morph = m_modelData->morphs[i];
        float       ratio = m_morphRatios[i];
        if (ratio == 0.f)
            continue;

        switch (morph.type)
        {
        case MorphType::Vertex:
            parallelForEach(morph.vertex, morph.vertex + morph.count,
                            [&](const VertexMorph &data)
                            {
                                renderData.setVertexPosition(
                                    data.index,
                                    renderData.getVertexPosition(data.index) +
                                        ratio * data.offset);
                            });
            break;
        case MorphType::Material:
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data  = morph.material[j];
                size_t      first = 0, last = renderData.materials.size();
                if (data.index >= 0)
                {
                    first = data.index;
                    last  = first + 1;
                }
                if (data.operation == 0) // multiply
                {
                    for (; first != last; ++first)
                    {
                        auto &mul = renderData.materials[first].mul;
                        mul.diffuse *=
                            glm::mix(glm::vec4(1.f), data.diffuse, ratio);
                        mul.specular *=
                            glm::mix(glm::vec3(1.f), data.specular, ratio);
                        mul.specularPower *=
                            glm::mix(1.f, data.specularPower, ratio);
                        mul.ambient *=
                            glm::mix(glm::vec3(1.f), data.ambient, ratio);
                        mul.edgeColor *=
                            glm::mix(glm::vec4(1.f), data.edgeColor, ratio);
                        mul.edgeSize *= glm::mix(1.f, data.edgeSize, ratio);
                        mul.texture *=
                            glm::mix(glm::vec4(1.f), data.texture, ratio);
                        mul.sphereTexture *=
                            glm::mix(glm::vec4(1.f), data.sphereTexture, ratio);
                        mul.toonTexture *=
                            glm::mix(glm::vec4(1.f), data.toonTexture, ratio);
                    }
                }
                else // add
                {
                    for (; first != last; ++first)
                    {
                        auto &add = renderData.materials[first].add;
                        add.diffuse += ratio * data.diffuse;
                        add.specular += ratio * data.specular;
                        add.specularPower += ratio * data.specularPower;
                        add.ambient += ratio * data.ambient;
                        add.edgeColor += ratio * data.edgeColor;
                        add.edgeSize += ratio * data.edgeSize;
                        add.texture += ratio * data.texture;
                        add.sphereTexture += ratio * data.sphereTexture;
                        add.toonTexture += ratio * data.toonTexture;
                    }
                }
            }
            break;
        case MorphType::UV:
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data = morph.uv[j];
                renderData.setVertexUV(data.index,
                                       renderData.getVertexUV(data.index) +
                                           ratio * glm::vec2(data.offset[0]));
            }
            break;
        case MorphType::UV1:
        case MorphType::UV2:
        case MorphType::UV3:
        case MorphType::UV4:
        {
            auto uvIndex = static_cast<uint8_t>(morph.type) -
                           static_cast<uint8_t>(MorphType::UV1);
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data = morph.uv[j];
                renderData.setVertexAdditionalUV(
                    data.index, uvIndex,
                    renderData.getVertexAdditionalUV(data.index, uvIndex) +
                        ratio * data.offset[1 + uvIndex]);
            }
        }
        break;
        default:
            break;
        }
    }

    renderData.applyMaterialFactors();
}

void ModelPose::applyBoneTransformsToRenderData(
    ModelRenderData &renderData) const
{
    std::vector<glm::mat4> finalBoneTransforms(m_globalBoneTransforms.size());
    for (uint32_t i = 0; i < finalBoneTransforms.size(); ++i)
        finalBoneTransforms[i] = getFinalBoneTransform(i);

    parallelForEach(
        m_modelData->vertices.cbegin(), m_modelData->vertices.cend(),
        [&](const Vertex &vert)
        {
            auto i   = static_cast<uint32_t>(&vert - &m_modelData->vertices[0]);
            auto pos = renderData.getVertexPosition(i);
            auto norm = renderData.getVertexNormal(i);

            glm::mat4 vertMatrix;

            switch (vert.skinningType)
            {
            case VertexSkinningType::BDEF1:
                vertMatrix = finalBoneTransforms[vert.boneIndices[0]];
                break;
            case VertexSkinningType::BDEF2:
                vertMatrix = vert.boneWeights[0] *
                                 finalBoneTransforms[vert.boneIndices[0]] +
                             (1.f - vert.boneWeights[0]) *
                                 finalBoneTransforms[vert.boneIndices[1]];
                break;
            case VertexSkinningType::BDEF4:
                vertMatrix = vert.boneWeights[0] *
                                 finalBoneTransforms[vert.boneIndices[0]] +
                             vert.boneWeights[1] *
                                 finalBoneTransforms[vert.boneIndices[1]] +
                             vert.boneWeights[2] *
                                 finalBoneTransforms[vert.boneIndices[2]] +
                             vert.boneWeights[3] *
                                 finalBoneTransforms[vert.boneIndices[3]];
                break;
            case VertexSkinningType::SDEF:
            {
                float w0 = vert.boneWeights[0];
                float w1 = 1.f - w0;
                auto  q0 = m_globalBoneTransforms[vert.boneIndices[0]].rotation;
                auto  q1 = m_globalBoneTransforms[vert.boneIndices[1]].rotation;
                auto  rot = glm::mat3_cast(glm::slerp(q0, q1, w1));

                const auto &c = vert.sdefC;
                auto        r = 0.5f * (vert.sdefR0 - vert.sdefR1);

                const auto &m0 = finalBoneTransforms[vert.boneIndices[0]];
                const auto &m1 = finalBoneTransforms[vert.boneIndices[1]];

                pos = rot * (pos - c) +
                      glm::vec3(m0 * glm::vec4(c + w1 * r, 1.f)) * w0 +
                      glm::vec3(m1 * glm::vec4(c - w0 * r, 1.f)) * w1;
                norm = glm::normalize(rot * norm);
            }
            break;
            case VertexSkinningType::QDEF:
            {
                glm::dualquat dq[4];
                glm::vec4     w(0.f);
                for (int bi = 0; bi < 4; ++bi)
                {
                    auto j = vert.boneIndices[bi];
                    if (j != -1)
                    {
                        dq[bi] = glm::dualquat_cast(glm::mat3x4(
                            glm::transpose(finalBoneTransforms[j])));
                        dq[bi] = glm::normalize(dq[bi]);
                        w[bi]  = vert.boneWeights[bi];
                    }
                    else
                        w[bi] = 0;
                }

                if (glm::dot(dq[0].real, dq[1].real) < 0)
                    w[1] *= -1.0f;
                if (glm::dot(dq[0].real, dq[2].real) < 0)
                    w[2] *= -1.0f;
                if (glm::dot(dq[0].real, dq[3].real) < 0)
                    w[3] *= -1.0f;
                auto blendDQ =
                    w[0] * dq[0] + w[1] * dq[1] + w[2] * dq[2] + w[3] * dq[3];
                blendDQ    = glm::normalize(blendDQ);
                vertMatrix = glm::transpose(glm::mat3x4_cast(blendDQ));
                break;
            }
            }
            if (vert.skinningType != VertexSkinningType::SDEF)
            {
                pos  = glm::vec3(vertMatrix * glm::vec4(pos, 1.f));
                norm = glm::mat3(vertMatrix) * norm;
            }
            renderData.setVertexPosition(i, pos);
            renderData.setVertexNormal(i, norm);
        });
}

void ModelPose::blendWith(const ModelPose &other, float t)
{
    for (size_t i = 0; i < m_localBoneTransforms.size(); ++i)
        m_localBoneTransforms[i] = ((1.f - t) * m_localBoneTransforms[i]) *
                                   (t * other.m_localBoneTransforms[i]);

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] =
            glm::mix(m_morphRatios[i], other.m_morphRatios[i], t);
}

void ModelPose::operator+=(const ModelPose &other)
{
    for (size_t i = 0; i < m_localBoneTransforms.size(); ++i)
        m_localBoneTransforms[i] =
            m_localBoneTransforms[i] * other.m_localBoneTransforms[i];

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] = other.m_morphRatios[i] + m_morphRatios[i];
}

void ModelPose::operator*=(float t)
{
    for (size_t i = 0; i < m_localBoneTransforms.size(); ++i)
        m_localBoneTransforms[i] *= t;

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] *= t;
}

} // namespace glmmd