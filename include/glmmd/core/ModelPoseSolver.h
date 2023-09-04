#ifndef GLMMD_CORE_MODEL_POSE_SOLVER_H_
#define GLMMD_CORE_MODEL_POSE_SOLVER_H_

#include <memory>

#include <glmmd/core/ModelPose.h>
#include <glmmd/core/ModelPhysics.h>

namespace glmmd
{

class ModelPoseSolver
{
public:
    ModelPoseSolver(const std::shared_ptr<const ModelData> &modelData);

    ModelPoseSolver(const ModelPoseSolver &other) = default;

    void solveBeforePhysics(ModelPose &pose);
    void syncWithPhysics(ModelPose &pose, ModelPhysics &physics);
    void solveAfterPhysics(ModelPose &pose);

private:
    void applyGroupMorphs(ModelPose &);
    void applyBoneMorphs(ModelPose &);

    void solveChildGlobalBoneTransforms(ModelPose &, uint32_t boneIndex);
    void solveGlobalBoneTransformsBeforePhysics(ModelPose &);
    void solveIK(ModelPose &);
    void updateInheritedBoneTransforms(ModelPose &);
    void solveGlobalBoneTransformsAfterPhysics(ModelPose &);

private:
    std::shared_ptr<const ModelData> m_modelData;

    std::vector<uint32_t> m_boneDeformOrder;
    uint32_t              m_afterPhysicsStartIndex;
};

} // namespace glmmd

#endif
