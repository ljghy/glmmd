#include <algorithm>
#include <numeric>
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
#include <execution>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>

#include <glmmd/core/ModelPose.h>

namespace glmmd
{

ModelPose::ModelPose(const std::shared_ptr<const ModelData> &modelData)
    : m_modelData(modelData)
    , m_localBoneTranslations(modelData->bones.size(), glm::vec3(0.f))
    , m_localBoneRotations(modelData->bones.size(),
                           glm::quat(1.f, 0.f, 0.f, 0.f))
    , m_morphRatios(modelData->morphs.size(), 0.f)
    , m_globalBoneTransforms(modelData->bones.size(), glm::mat4(1.f))
{
}

glm::vec3 ModelPose::getGlobalBonePosition(uint32_t boneIndex) const
{
    return glm::vec3(m_globalBoneTransforms[boneIndex][3]);
}

void ModelPose::setLocalBoneTranslation(uint32_t         boneIndex,
                                        const glm::vec3 &translation)
{
    m_localBoneTranslations[boneIndex] = translation;
}

void ModelPose::setLocalBoneRotation(uint32_t         boneIndex,
                                     const glm::quat &rotation)
{
    m_localBoneRotations[boneIndex] = rotation;
}

void ModelPose::setMorphRatio(uint32_t morphIndex, float ratio)
{
    m_morphRatios[morphIndex] = ratio;
}

void ModelPose::setGlobalBoneTransform(uint32_t         boneIndex,
                                       const glm::mat4 &transform)
{
    m_globalBoneTransforms[boneIndex] = transform;
}

const glm::mat4 &ModelPose::getGlobalBoneTransform(uint32_t boneIndex) const
{
    return m_globalBoneTransforms[boneIndex];
}

glm::mat4 ModelPose::getFinalBoneTransform(uint32_t boneIndex) const
{
    return glm::translate(m_globalBoneTransforms[boneIndex],
                          -m_modelData->bones[boneIndex].position);
}

const glm::vec3 &ModelPose::getLocalBoneTranslation(uint32_t boneIndex) const
{
    return m_localBoneTranslations[boneIndex];
}

const glm::quat &ModelPose::getLocalBoneRotation(uint32_t boneIndex) const
{
    return m_localBoneRotations[boneIndex];
}

float ModelPose::getMorphRatio(uint32_t morphIndex) const
{
    return m_morphRatios[morphIndex];
}

void ModelPose::resetLocal()
{
    std::fill(m_localBoneTranslations.begin(), m_localBoneTranslations.end(),
              glm::vec3(0.f));
    std::fill(m_localBoneRotations.begin(), m_localBoneRotations.end(),
              glm::quat(1.f, 0.f, 0.f, 0.f));
    std::fill(m_morphRatios.begin(), m_morphRatios.end(), 0.f);
}

void ModelPose::applyToRenderData(RenderData &renderData) const
{
    applyMorphsToRenderData(renderData);
    applyBoneTransformsToRenderData(renderData);
}

void ModelPose::applyMorphsToRenderData(RenderData &renderData) const
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
            std::for_each(
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
                std::execution::par,
#endif
                morph.vertex, morph.vertex + morph.count,
                [&](const VertexMorph &data) {
                    renderData.vertexBuffer[data.index].position +=
                        ratio * data.offset;
                });
            break;
        case MorphType::Material:
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data = morph.material[j];
                if (data.operation == 0) // multiply
                {
                    auto &mat = renderData.materialMul[data.index];
                    mat.diffuse *=
                        glm::mix(glm::vec4(1.f), data.diffuse, ratio);
                    mat.specular *=
                        glm::mix(glm::vec3(1.f), data.specular, ratio);
                    mat.specularPower *=
                        glm::mix(1.f, data.specularPower, ratio);
                    mat.ambient *=
                        glm::mix(glm::vec3(1.f), data.ambient, ratio);
                    mat.edgeColor *=
                        glm::mix(glm::vec4(1.f), data.edgeColor, ratio);
                    mat.edgeSize *= glm::mix(1.f, data.edgeSize, ratio);
                    mat.texture *=
                        glm::mix(glm::vec4(1.f), data.texture, ratio);
                    mat.sphereTexture *=
                        glm::mix(glm::vec4(1.f), data.sphereTexture, ratio);
                    mat.toonTexture *=
                        glm::mix(glm::vec4(1.f), data.toonTexture, ratio);
                }
                else // add
                {
                    auto &mat = renderData.materialAdd[data.index];
                    mat.diffuse += ratio * data.diffuse;
                    mat.specular += ratio * data.specular;
                    mat.specularPower += ratio * data.specularPower;
                    mat.ambient += ratio * data.ambient;
                    mat.edgeColor += ratio * data.edgeColor;
                    mat.edgeSize += ratio * data.edgeSize;
                    mat.texture += ratio * data.texture;
                    mat.sphereTexture += ratio * data.sphereTexture;
                    mat.toonTexture += ratio * data.toonTexture;
                }
            }
            break;
        case MorphType::UV:
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data = morph.uv[j];
                renderData.vertexBuffer[data.index].uv +=
                    ratio * glm::vec2(data.offset[0]);
            }
            break;
#if 0
            // TODO:
        case MorphType::UV1:
        case MorphType::UV2:
        case MorphType::UV3:
        case MorphType::UV4:
        {
            auto uvChannel = static_cast<uint8_t>(morph.type) -
                             static_cast<uint8_t>(MorphType::UV1);
            for (const auto &d : morph.data)
            {
                const auto &data = std::get<1>(d);
                renderData.additionalUVs[uvChannel][data.index] +=
                    ratio * data.offset[uvChannel];
            }
        }
        break;
#endif
        default:
            break;
        }
    }

    renderData.applyMaterialFactors();
}

void ModelPose::applyBoneTransformsToRenderData(RenderData &renderData) const
{
    std::vector<glm::mat4> finalBoneTransforms(m_globalBoneTransforms.size());
    for (size_t i = 0; i < finalBoneTransforms.size(); ++i)
        finalBoneTransforms[i] = getFinalBoneTransform(i);

    std::for_each(
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
        std::execution::par,
#endif
        m_modelData->vertices.cbegin(), m_modelData->vertices.cend(),
        [&](const Vertex &vert)
        {
            auto  i = static_cast<uint32_t>(&vert - &m_modelData->vertices[0]);
            auto &pos  = renderData.vertexBuffer[i].position;
            auto &norm = renderData.vertexBuffer[i].normal;

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
                auto  q0 =
                    glm::quat_cast(m_globalBoneTransforms[vert.boneIndices[0]]);
                auto q1 =
                    glm::quat_cast(m_globalBoneTransforms[vert.boneIndices[1]]);
                auto rot = glm::mat3_cast(glm::slerp(q0, q1, w1));

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
        });
}

void ModelPose::blendWith(const ModelPose &other, float t)
{
    for (size_t i = 0; i < m_localBoneTranslations.size(); ++i)
    {
        m_localBoneTranslations[i] = glm::mix(
            m_localBoneTranslations[i], other.m_localBoneTranslations[i], t);
        m_localBoneRotations[i] = glm::slerp(m_localBoneRotations[i],
                                             other.m_localBoneRotations[i], t);
    }

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] =
            glm::mix(m_morphRatios[i], other.m_morphRatios[i], t);
}

void ModelPose::operator+=(const ModelPose &other)
{
    for (size_t i = 0; i < m_localBoneTranslations.size(); ++i)
    {
        m_localBoneTranslations[i] =
            other.m_localBoneTranslations[i] + m_localBoneTranslations[i];
        m_localBoneRotations[i] =
            other.m_localBoneRotations[i] * m_localBoneRotations[i];
    }

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] = other.m_morphRatios[i] + m_morphRatios[i];
}

void ModelPose::operator*=(float t)
{
    for (size_t i = 0; i < m_localBoneTranslations.size(); ++i)
    {
        m_localBoneTranslations[i] *= t;
        m_localBoneRotations[i] = glm::slerp(glm::quat(1.f, 0.f, 0.f, 0.f),
                                             m_localBoneRotations[i], t);
    }

    for (size_t i = 0; i < m_morphRatios.size(); ++i)
        m_morphRatios[i] *= t;
}

} // namespace glmmd