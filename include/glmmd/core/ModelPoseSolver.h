#ifndef GLMMD_CORE_MODEL_POSE_SOLVER_H_
#define GLMMD_CORE_MODEL_POSE_SOLVER_H_

#include <memory>

#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/ModelPose.h>

namespace glmmd
{

class ModelPoseSolver
{
public:
    ModelPoseSolver() = default;
    ModelPoseSolver(const std::shared_ptr<const ModelData> &modelData);
    ModelPoseSolver(const ModelPoseSolver &)                = default;
    ModelPoseSolver &operator=(const ModelPoseSolver &)     = default;
    ModelPoseSolver(ModelPoseSolver &&) noexcept            = default;
    ModelPoseSolver &operator=(ModelPoseSolver &&) noexcept = default;

    void create(const std::shared_ptr<const ModelData> &modelData);

    void solveBeforePhysics(ModelPose &pose) const;
    void syncWithPhysics(ModelPose &pose, ModelPhysics &physics) const;
    void solveAfterPhysics(ModelPose &pose) const;

private:
    void sortBoneDeformOrder();

    void applyGroupMorphs(ModelPose &) const;
    void applyBoneMorphs(ModelPose &) const;

    bool solveChildGlobalBoneTransforms(ModelPose &, uint32_t boneIndex,
                                        int32_t stop = -1) const;
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
