#ifndef GLMMD_CORE_POSE_SOLVER_H_
#define GLMMD_CORE_POSE_SOLVER_H_

#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/Pose.h>

#include <vector>

namespace glmmd {

struct RigidBodyData;

// Resolves a Pose's local bone transforms into global transforms, applying
// group/bone morphs, inheritance, IK, and physics synchronization. Deform order
// is precomputed from physics phase, deform layer, and bone index. IK executes
// at each controller's position in this order. A typical
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
  void applyBoneMorphs(Pose &) const;

  void updateGlobalTransform(Pose &, uint32_t boneIndex) const;
  void ensureGlobalTransform(Pose &, uint32_t boneIndex) const;
  void updateGlobalTransforms(Pose &) const;

  void solveIK(Pose &, uint32_t ikIndex) const;
  void updateInheritedTransform(Pose &, uint32_t boneIndex) const;
  void solveBoneRange(Pose &, uint32_t first, uint32_t last) const;

  struct IKLinkPlan {
    // Root-to-parent path, including hierarchy bones absent from the IK links.
    std::vector<uint32_t> parentPath;
    int hingeAxis = -1;
    bool fixed = false;
  };

  struct IKPlan {
    std::vector<uint32_t> effectorPath;
    std::vector<IKLinkPlan> links;
    uint32_t scratchOffset = 0;
    float positionTolerance = 1e-5f;
  };

  void syncStaticRigidBodyTransforms(const Pose &pose, const RigidBodyData &rb,
                                     int32_t bi) const;

  void syncDynamicRigidBodyTransforms(Pose &pose, const RigidBodyData &rb,
                                      int32_t bi) const;

  void syncMixedRigidBodyTransforms(Pose &pose, const RigidBodyData &rb,
                                    int32_t bi) const;

private:
  const ModelData *m_modelData = nullptr;

  std::vector<uint32_t> m_hierarchyOrder;
  std::vector<uint32_t> m_boneDeformOrder;
  uint32_t m_afterPhysicsOffset = 0;

  std::vector<IKPlan> m_ikPlans;

  // Precomputed static offset of each bone from its parent (bone.position -
  // parent.position, or bone.position for roots). Constant for a given model,
  // so computed once in create() rather than every solve.
  std::vector<glm::vec3> m_boneLocalOffsets;

  bool m_enableIK = true;
};

} // namespace glmmd

#endif
