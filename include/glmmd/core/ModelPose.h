#ifndef GLMMD_CORE_MODEL_POSE_H_
#define GLMMD_CORE_MODEL_POSE_H_

#include <memory>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/RenderData.h>

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

    void setLocalBoneTranslation(uint32_t         boneIndex,
                                 const glm::vec3 &translation);
    void setLocalBoneRotation(uint32_t boneIndex, const glm::quat &rotation);
    void setMorphRatio(uint32_t morphIndex, float ratio);

    void setGlobalBoneTransform(uint32_t boneIndex, const glm::mat4 &transform);
    const glm::mat4 &getGlobalBoneTransform(uint32_t boneIndex) const;
    glm::vec3        getGlobalBonePosition(uint32_t boneIndex) const;

    const glm::vec3 &getLocalBoneTranslation(uint32_t boneIndex) const;
    const glm::quat &getLocalBoneRotation(uint32_t boneIndex) const;
    float            getMorphRatio(uint32_t morphIndex) const;

private:
    std::shared_ptr<const ModelData> m_modelData;

    std::vector<glm::vec3> m_localBoneTranslations;
    std::vector<glm::quat> m_localBoneRotations;
    std::vector<float>     m_morphRatios;

    std::vector<glm::mat4> m_globalBoneTransforms;
};

} // namespace glmmd

#endif
