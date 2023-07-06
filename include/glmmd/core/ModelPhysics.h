#ifndef GLMMD_CORE_MODEL_PHYSICS_H_
#define GLMMD_CORE_MODEL_PHYSICS_H_

#include <memory>
#include <vector>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include <glmmd/core/ModelPose.h>

namespace glmmd
{

struct RigidBodyData
{
    glm::vec3                             translationOffset;
    glm::quat                             rotationOffset;
    std::unique_ptr<btCollisionShape>     shape;
    std::unique_ptr<btDefaultMotionState> motionState;
    std::unique_ptr<btRigidBody>          rigidBody;
};

using JointData = std::unique_ptr<btGeneric6DofSpringConstraint>;

struct ModelPhysics
{
    std::vector<RigidBodyData> rigidBodies;
    std::vector<JointData>     joints;
};

}; // namespace glmmd

#endif
