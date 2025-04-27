#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

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

glm::dualquat ModelPose::getFinalBoneTransform(uint32_t boneIndex) const
{
    auto &r = m_globalBoneTransforms[boneIndex].rotation;
    auto &t = m_globalBoneTransforms[boneIndex].translation;
    return glm::dualquat(r, t - r * m_modelData->bones[boneIndex].position);
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
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &data = morph.vertex[j];
                renderData.setVertexPosition(
                    data.index, renderData.getVertexPosition(data.index) +
                                    ratio * data.offset);
            }
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
    std::vector<glm::dualquat> finalBoneTransforms(
        m_globalBoneTransforms.size());
    for (uint32_t i = 0; i < finalBoneTransforms.size(); ++i)
        finalBoneTransforms[i] = getFinalBoneTransform(i);

    parallelForEach(
        m_modelData->vertices.begin(), m_modelData->vertices.end(),
        [&](const Vertex &vert)
        {
            auto i   = static_cast<uint32_t>(&vert - &m_modelData->vertices[0]);
            auto pos = renderData.getVertexPosition(i);
            auto norm = renderData.getVertexNormal(i);

            if (vert.skinningType == VertexSkinningType::SDEF)
            {
                const auto &dq0 = finalBoneTransforms[vert.boneIndices[0]];
                const auto &dq1 = finalBoneTransforms[vert.boneIndices[1]];
                const auto &q0  = dq0.real;
                const auto &q1  = dq1.real;

                float w0 = vert.boneWeights[0];
                float w1 = 1.f - w0;

                auto q = glm::slerp(q0, q1, w1);

                const auto &c = vert.sdefC;

                auto r = 0.5f * (vert.sdefR0 - vert.sdefR1);

                pos = q * (pos - c) + (dq0 * (c + w1 * r)) * w0 +
                      (dq1 * (c - w0 * r)) * w1;
                norm = q * norm;
            }
            else
            {
                int nb =
                    vert.skinningType == VertexSkinningType::BDEF1
                        ? 1
                        : (vert.skinningType == VertexSkinningType::BDEF2 ? 2
                                                                          : 4);

                glm::dualquat dq = finalBoneTransforms[vert.boneIndices[0]];
                auto          q0 = dq.real;

                if (nb > 1)
                {
                    dq *= vert.boneWeights[0];
                    for (int bi = 1; bi < nb; ++bi)
                    {
                        float w = vert.boneWeights[bi];
                        if (glm::dot(q0,
                                     finalBoneTransforms[vert.boneIndices[bi]]
                                         .real) < 0)
                            w = -w;
                        dq = dq + w * finalBoneTransforms[vert.boneIndices[bi]];
                    }

                    dq = glm::normalize(dq);
                }

                pos  = dq * pos;
                norm = dq.real * norm;
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