#ifndef GLMMD_CORE_POSE_H_
#define GLMMD_CORE_POSE_H_

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelRenderData.h>
#include <glmmd/core/Transform.h>

#include <glm/glm.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>

namespace glmmd {

// A single frame of animation state for a model: local bone transforms and
// morph weights (the animated inputs), plus the global bone transforms computed
// from them by PoseSolver. Motions write the local state; PoseSolver fills the
// global state; ModelRenderData consumes it via applyToRenderData.
//
// A Pose holds a non-owning pointer to its ModelData; the referenced ModelData
// must outlive the Pose (and any copies of it).
class Pose {
  friend class PoseSolver;

public:
  Pose() = default;
  Pose(const ModelData &modelData);
  Pose(const Pose &) = default;
  Pose &operator=(const Pose &) = default;
  Pose(Pose &&) noexcept = default;
  Pose &operator=(Pose &&) noexcept = default;

  void create(const ModelData &modelData);

  void resetLocal();

  void applyToRenderData(ModelRenderData &renderData) const;
  void applyBoneTransformsToRenderData(ModelRenderData &renderData) const;
  void applyMorphsToRenderData(ModelRenderData &renderData) const;

  // Linear interpolation toward `other` by t in [0, 1].
  void blendWith(const Pose &other, float t);
  // Layer `other` on top of this pose: composes local bone transforms and adds
  // morph weights. Used to accumulate multiple motions.
  void compose(const Pose &other);
  // Scale this pose's contribution by t: rotations slerp from identity,
  // translations and morph weights scale linearly.
  void scale(float t);

  // Local (animated) bone state. Mutable; the source of truth a Motion writes.
  [[nodiscard]] const Transform &localBoneTransform(uint32_t boneIndex) const;
  [[nodiscard]] Transform &localBoneTransform(uint32_t boneIndex);

  [[nodiscard]] const glm::vec3 &localBoneTranslation(uint32_t boneIndex) const;
  [[nodiscard]] glm::vec3 &localBoneTranslation(uint32_t boneIndex);

  [[nodiscard]] const glm::quat &localBoneRotation(uint32_t boneIndex) const;
  [[nodiscard]] glm::quat &localBoneRotation(uint32_t boneIndex);

  // Morph weights. Mutable.
  [[nodiscard]] float morphWeight(uint32_t morphIndex) const;
  [[nodiscard]] float &morphWeight(uint32_t morphIndex);

  // Global bone state. Computed by PoseSolver; read-only.
  [[nodiscard]] const Transform &globalBoneTransform(uint32_t boneIndex) const;
  [[nodiscard]] glm::vec3 globalBonePosition(uint32_t boneIndex) const;
  [[nodiscard]] glm::dualquat finalBoneTransform(uint32_t boneIndex) const;

private:
  const ModelData *m_modelData = nullptr;

  std::vector<Transform> m_localBoneTransforms;
  std::vector<float> m_morphWeights;

  std::vector<Transform> m_globalBoneTransforms;

  // Scratch buffer reused across applyBoneTransformsToRenderData calls to avoid
  // a per-frame allocation. Sized to the bone count in create().
  mutable std::vector<glm::dualquat> m_finalBoneTransforms;
};

} // namespace glmmd

#endif
