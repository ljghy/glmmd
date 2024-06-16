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

    void sortBoneDeformOrder();

    void solveBeforePhysics(ModelPose &pose) const;
    void syncWithPhysics(ModelPose &pose, ModelPhysics &physics) const;
    void solveAfterPhysics(ModelPose &pose) const;

private:
    void applyGroupMorphs(ModelPose &) const;
    void applyBoneMorphs(ModelPose &) const;

    void solveChildGlobalBoneTransforms(ModelPose &, uint32_t boneIndex) const;
    void solveGlobalBoneTransforms(ModelPose &, uint32_t, uint32_t) const;
    void solveIK(ModelPose &, uint32_t, uint32_t) const;
    void updateInheritedBoneTransforms(ModelPose &, uint32_t, uint32_t) const;

    void syncStaticRigidBodyTransforms(const ModelPose     &pose,
                                       const RigidBodyData &rb,
                                       int32_t              bi) const;

    void syncDynamicRigidBodyTransforms(ModelPose           &pose,
                                        const RigidBodyData &rb,
                                        int32_t              bi) const;

    void syncMixedRigidBodyTransforms(ModelPose &pose, const RigidBodyData &rb,
                                      int32_t bi) const;

private:
    std::shared_ptr<const ModelData> m_modelData;

    std::vector<std::pair<uint32_t, uint32_t>> m_updateBeforePhysicsRanges;
    std::vector<std::pair<uint32_t, uint32_t>> m_updateAfterPhysicsRanges;

    std::vector<std::vector<uint32_t>> m_boneChildren;
    std::vector<uint32_t>              m_boneDeformOrder;
};

} // namespace glmmd

#endif
