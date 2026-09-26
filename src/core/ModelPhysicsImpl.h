#ifndef GLMMD_CORE_MODEL_PHYSICS_IMPL_H_
#define GLMMD_CORE_MODEL_PHYSICS_IMPL_H_

// Internal header: Bullet backend state, not installed. Public code only sees
// glmmd/core/ModelPhysics.h.

#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/Transform.h>

#include <memory>
#include <vector>

#if !defined(GLMMD_DONT_USE_BULLET)
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#endif

namespace glmmd {

struct RigidBodyData {
  Transform offset;

#if !defined(GLMMD_DONT_USE_BULLET)
  std::unique_ptr<btCollisionShape> shape;
  std::unique_ptr<btDefaultMotionState> motionState;
  std::unique_ptr<btRigidBody> rigidBody;
#endif
};

#if !defined(GLMMD_DONT_USE_BULLET)
using JointData = std::unique_ptr<btGeneric6DofSpringConstraint>;
#else
struct JointData {};
#endif

class ModelPhysicsImpl {
public:
  std::vector<RigidBodyData> rigidBodies;
  std::vector<JointData> joints;
};

} // namespace glmmd

#endif
