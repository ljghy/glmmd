#ifndef GLMMD_CORE_PHYSICS_WORLD_H_
#define GLMMD_CORE_PHYSICS_WORLD_H_

#include <glmmd/core/Model.h>

#include <glm/glm.hpp>

#include <memory>

namespace glmmd {

class PhysicsWorldImpl;

// A dynamics world that simulates one or more models' rigid bodies and joints.
// The Bullet backend is hidden behind an opaque implementation pointer so this
// header stays free of Bullet includes.
class PhysicsWorld {
public:
  PhysicsWorld();
  ~PhysicsWorld();

  PhysicsWorld(PhysicsWorld &&) noexcept;
  PhysicsWorld &operator=(PhysicsWorld &&) noexcept;

  PhysicsWorld(const PhysicsWorld &) = delete;
  PhysicsWorld &operator=(const PhysicsWorld &) = delete;

  void setupModelPhysics(Model &model, bool applyCurrentTransforms = false);

  void clearModelPhysics(Model &model);

  void update(float deltaTime, int maxSubSteps = 10,
              float fixedDeltaTime = 1.f / 60.f);

  void setGravity(const glm::vec3 &gravity);

private:
  void setupModelRigidBodies(Model &model, bool applyCurrentTransforms);
  void setupModelJoints(Model &model);

  std::unique_ptr<PhysicsWorldImpl> m_impl;
};

} // namespace glmmd

#endif
