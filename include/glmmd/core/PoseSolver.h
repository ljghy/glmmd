#ifndef GLMMD_CORE_POSE_SOLVER_H_
#define GLMMD_CORE_POSE_SOLVER_H_

#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/Pose.h>

#include <vector>

namespace glmmd {

struct RigidBodyData;

// Resolves a Pose's local bone transforms into global transforms, applying
// group/bone morphs, inheritance, IK, and physics synchronization. Deform order
// is precomputed from the model's bone hierarchy and deform layers. A typical
// frame runs solveBeforePhysics -> syncWithPhysics -> solveAfterPhysics
// (see Model::solvePose).
//
// A PoseSolver holds a non-owning pointer to its ModelData; the referenced
// ModelData must outlive the PoseSolver.
class PoseSolver {
public:
  PoseSolver() = default;
  PoseSolver(const ModelData &modelData);
  PoseSolver(const PoseSolver &) = default;
  PoseSolver &operator=(const PoseSolver &) = default;
  PoseSolver(PoseSolver &&) noexcept = default;
  PoseSolver &operator=(PoseSolver &&) noexcept = default;

  void create(const ModelData &modelData);

  void solveBeforePhysics(Pose &pose) const;
  void syncWithPhysics(Pose &pose, ModelPhysics &physics) const;
  void solveAfterPhysics(Pose &pose) const;

  void enableIK(bool enable) { m_enableIK = enable; }
  bool isIKEnabled() const { return m_enableIK; }

private:
  void sortBoneDeformOrder();

  void applyGroupMorphs(Pose &) const;
  void applyBoneMorphs(Pose &) const;

  bool solveChildGlobalBoneTransforms(Pose &, uint32_t boneIndex,
                                      int32_t stop = -1) const;
  void solveGlobalBoneTransforms(Pose &, uint32_t, uint32_t) const;
  void solveIK(Pose &, uint32_t, uint32_t) const;
  void updateInheritedBoneTransforms(Pose &, uint32_t, uint32_t) const;

  void solveDeformRanges(
      Pose &, const std::vector<std::pair<uint32_t, uint32_t>> &ranges) const;

  void syncStaticRigidBodyTransforms(const Pose &pose, const RigidBodyData &rb,
                                     int32_t bi) const;

  void syncDynamicRigidBodyTransforms(Pose &pose, const RigidBodyData &rb,
                                      int32_t bi) const;

  void syncMixedRigidBodyTransforms(Pose &pose, const RigidBodyData &rb,
                                    int32_t bi) const;

private:
  const ModelData *m_modelData = nullptr;

  std::vector<std::pair<uint32_t, uint32_t>> m_updateBeforePhysicsRanges;
  std::vector<std::pair<uint32_t, uint32_t>> m_updateAfterPhysicsRanges;

  std::vector<std::vector<uint32_t>> m_boneChildren;
  std::vector<uint32_t> m_boneDeformOrder;

  // Precomputed static offset of each bone from its parent (bone.position -
  // parent.position, or bone.position for roots). Constant for a given model,
  // so computed once in create() rather than every solve.
  std::vector<glm::vec3> m_boneLocalOffsets;

  bool m_enableIK = true;
};

} // namespace glmmd

#endif
