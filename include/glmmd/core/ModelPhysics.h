#ifndef GLMMD_CORE_MODEL_PHYSICS_H_
#define GLMMD_CORE_MODEL_PHYSICS_H_

#include <memory>

namespace glmmd {

class ModelPhysicsImpl;

// Opaque container for a model's physics simulation state (rigid bodies and
// joints). The concrete representation depends on the physics backend (Bullet)
// and is deliberately kept out of the public API so that including this header
// does not pull in Bullet. Populated and driven by PhysicsWorld.
class ModelPhysics {
public:
  ModelPhysics();
  ~ModelPhysics();

  ModelPhysics(ModelPhysics &&) noexcept;
  ModelPhysics &operator=(ModelPhysics &&) noexcept;

  ModelPhysics(const ModelPhysics &) = delete;
  ModelPhysics &operator=(const ModelPhysics &) = delete;

private:
  friend class PoseSolver;
  friend class PhysicsWorld;

  std::unique_ptr<ModelPhysicsImpl> m_impl;
};

} // namespace glmmd

#endif
