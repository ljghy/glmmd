#ifndef GLMMD_CORE_MODEL_POSE_H_
#define GLMMD_CORE_MODEL_POSE_H_

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelRenderData.h>
#include <glmmd/core/Transform.h>

namespace glmmd
{

class ModelPose
{
    friend class ModelPoseSolver;

public:
    ModelPose() = default;
    ModelPose(const std::shared_ptr<const ModelData> &modelData);
    ModelPose(const ModelPose &)                = default;
    ModelPose &operator=(const ModelPose &)     = default;
    ModelPose(ModelPose &&) noexcept            = default;
    ModelPose &operator=(ModelPose &&) noexcept = default;

    void create(const std::shared_ptr<const ModelData> &modelData);

    void resetLocal();

    void applyToRenderData(ModelRenderData &renderData) const;
    void applyBoneTransformsToRenderData(ModelRenderData &renderData) const;
    void applyMorphsToRenderData(ModelRenderData &renderData) const;

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

    glm::dualquat getFinalBoneTransform(uint32_t boneIndex) const;

    const glm::vec3 &getLocalBoneTranslation(uint32_t boneIndex) const;
    glm::vec3       &localBoneTranslation(uint32_t boneIndex);

    const glm::quat &getLocalBoneRotation(uint32_t boneIndex) const;
    glm::quat       &localBoneRotation(uint32_t boneIndex);

    float  getMorphRatio(uint32_t morphIndex) const;
    float &morphRatio(uint32_t morphIndex);

private:
    std::shared_ptr<const ModelData> m_modelData;

    std::vector<Transform> m_localBoneTransforms;
    std::vector<float>     m_morphRatios;

    std::vector<Transform> m_globalBoneTransforms;
};

} // namespace glmmd

#endif
