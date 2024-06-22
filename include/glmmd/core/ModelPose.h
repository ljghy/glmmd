#ifndef GLMMD_CORE_MODEL_POSE_H_
#define GLMMD_CORE_MODEL_POSE_H_

#include <memory>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/RenderData.h>
#include <glmmd/core/Transform.h>

namespace glmmd
{

class ModelPose
{
    friend class ModelPoseSolver;

public:
    ModelPose(const std::shared_ptr<const ModelData> &modelData);
    ModelPose(const ModelPose &other) = default;

    void resetLocal();

    void applyToRenderData(RenderData &renderData) const;
    void applyBoneTransformsToRenderData(RenderData &renderData) const;
    void applyMorphsToRenderData(RenderData &renderData) const;

    void blendWith(const ModelPose &other, float t);
    void operator+=(const ModelPose &other);
    void operator*=(float t);

    void setLocalBoneTransform(uint32_t boneIndex, const Transform &transform);
    void setLocalBoneTranslation(uint32_t         boneIndex,
                                 const glm::vec3 &translation);
    void setLocalBoneRotation(uint32_t boneIndex, const glm::quat &rotation);
    void setMorphRatio(uint32_t morphIndex, float ratio);

    const Transform &getGlobalBoneTransform(uint32_t boneIndex) const;
    glm::vec3        getGlobalBonePosition(uint32_t boneIndex) const;

    glm::mat4 getFinalBoneTransform(uint32_t boneIndex) const;

    const glm::vec3 &getLocalBoneTranslation(uint32_t boneIndex) const;
    const glm::quat &getLocalBoneRotation(uint32_t boneIndex) const;
    float            getMorphRatio(uint32_t morphIndex) const;

private:
    std::shared_ptr<const ModelData> m_modelData;

    std::vector<Transform> m_localBoneTransforms;
    std::vector<float>     m_morphRatios;

    std::vector<Transform> m_globalBoneTransforms;
};

} // namespace glmmd

#endif
