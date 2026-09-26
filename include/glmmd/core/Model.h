#ifndef GLMMD_CORE_MODEL_H_
#define GLMMD_CORE_MODEL_H_

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/Pose.h>
#include <glmmd/core/PoseSolver.h>

#include <memory>

namespace glmmd {

// A loaded model instance: shared, immutable source data plus the per-instance
// mutable state needed to animate and simulate it. Move-only, since it owns
// physics state. Construct one Model per animated instance; multiple Models may
// share the same ModelData.
//
// `data` owns the ModelData (shared, immutable); pose/poseSolver hold
// non-owning pointers into it. This stays valid across a Model move because the
// pointee is heap-stable (owned by the shared_ptr).
struct Model {
  Model(const std::shared_ptr<const ModelData> &data)
      : data(data), pose(*data), poseSolver(*data) {
    pose.resetLocal();
    poseSolver.solveBeforePhysics(pose);
    poseSolver.solveAfterPhysics(pose);
  }

  Model(const Model &other) = delete;
  Model &operator=(const Model &other) = delete;

  Model(Model &&other) noexcept = default;
  Model &operator=(Model &&other) noexcept = default;

  void solvePose() {
    poseSolver.solveBeforePhysics(pose);
    poseSolver.syncWithPhysics(pose, physics);
    poseSolver.solveAfterPhysics(pose);
  }

  std::shared_ptr<const ModelData> data;
  ModelPhysics physics;
  Pose pose;
  PoseSolver poseSolver;
};

} // namespace glmmd

#endif