#ifndef GLMMD_CORE_MODEL_PHYSICS_H_
#define GLMMD_CORE_MODEL_PHYSICS_H_

#include <vector>

#ifndef GLMMD_DONT_USE_BULLET
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <memory>
#endif

#include <glmmd/core/ModelPose.h>
#include <glmmd/core/Transform.h>

namespace glmmd
{

struct RigidBodyData
{
    Transform offset;

#ifndef GLMMD_DONT_USE_BULLET
    std::unique_ptr<btCollisionShape>     shape;
    std::unique_ptr<btDefaultMotionState> motionState;
    std::unique_ptr<btRigidBody>          rigidBody;
#endif
};

#ifndef GLMMD_DONT_USE_BULLET
using JointData = std::unique_ptr<btGeneric6DofSpringConstraint>;
#else
struct JointData
{
};
#endif

struct ModelPhysics
{
    std::vector<RigidBodyData> rigidBodies;
    std::vector<JointData>     joints;
};

}; // namespace glmmd

#endif
