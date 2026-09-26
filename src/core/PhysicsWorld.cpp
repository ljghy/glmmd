#include <glmmd/core/PhysicsWorld.h>

#include "ModelPhysicsImpl.h"

#if !defined(GLMMD_DONT_USE_BULLET)

#include <glm/gtx/euler_angles.hpp>

namespace glmmd {

static constexpr float GRAVITY_SCALE = 12.5f;

inline static btVector3 glm2btVector3(const glm::vec3 &v) {
  return {v.x, v.y, v.z};
}

inline static btQuaternion glm2btQuaternion(const glm::quat &q) {
  return {q.x, q.y, q.z, q.w};
}

inline static btTransform glm2btTransform(const Transform &transform) {
  btTransform t;
  t.setOrigin(glm2btVector3(transform.translation));
  t.setRotation(glm2btQuaternion(transform.rotation));
  return t;
}

inline static btMatrix3x3 eulerAnglesToMatrix(const glm::vec3 &eulerAngles) {
  glm::mat4 rot =
      glm::eulerAngleYXZ(eulerAngles.y, eulerAngles.x, eulerAngles.z);
  btMatrix3x3 m;
  m.setIdentity();
  m.setFromOpenGLSubMatrix(&rot[0][0]);
  return m;
}

class PhysicsWorldImpl {
public:
  std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
  std::unique_ptr<btCollisionDispatcher> dispatcher;
  std::unique_ptr<btBroadphaseInterface> broadphase;
  std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
  std::unique_ptr<btDiscreteDynamicsWorld> world;

  std::unique_ptr<btCollisionShape> groundShape;
  std::unique_ptr<btDefaultMotionState> groundMotionState;
  std::unique_ptr<btRigidBody> groundRigidBody;

  btVector3 gravity;
};

PhysicsWorld::PhysicsWorld() : m_impl(std::make_unique<PhysicsWorldImpl>()) {
  m_impl->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
  m_impl->dispatcher =
      std::make_unique<btCollisionDispatcher>(m_impl->collisionConfig.get());
  m_impl->broadphase = std::make_unique<btDbvtBroadphase>();
  m_impl->solver = std::make_unique<btSequentialImpulseConstraintSolver>();
  m_impl->world = std::make_unique<btDiscreteDynamicsWorld>(
      m_impl->dispatcher.get(), m_impl->broadphase.get(), m_impl->solver.get(),
      m_impl->collisionConfig.get());
  m_impl->groundShape =
      std::make_unique<btStaticPlaneShape>(btVector3(0.f, 1.f, 0.f), 0.f);
  m_impl->gravity = btVector3(0.f, -9.8f, 0.f) * GRAVITY_SCALE;

  m_impl->world->setGravity(m_impl->gravity);

  btTransform groundTransform;
  groundTransform.setIdentity();
  m_impl->groundMotionState =
      std::make_unique<btDefaultMotionState>(groundTransform);

  btRigidBody::btRigidBodyConstructionInfo groundInfo(
      0.f, m_impl->groundMotionState.get(), m_impl->groundShape.get(),
      btVector3(0.f, 0.f, 0.f));
  m_impl->groundRigidBody = std::make_unique<btRigidBody>(groundInfo);

  m_impl->world->addRigidBody(m_impl->groundRigidBody.get(), 1 << 15, 0x7FFF);
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld &&) noexcept = default;
PhysicsWorld &PhysicsWorld::operator=(PhysicsWorld &&) noexcept = default;

void PhysicsWorld::update(float deltaTime, int maxSubSteps,
                          float fixedDeltaTime) {
  m_impl->world->stepSimulation(deltaTime, maxSubSteps, fixedDeltaTime);
}

void PhysicsWorld::setGravity(const glm::vec3 &gravity) {
  m_impl->gravity = glm2btVector3(gravity) * GRAVITY_SCALE;
  m_impl->world->setGravity(m_impl->gravity);
}

void PhysicsWorld::setupModelPhysics(Model &model,
                                     bool applyCurrentTransforms) {
  setupModelRigidBodies(model, applyCurrentTransforms);
  setupModelJoints(model);
}

void PhysicsWorld::setupModelRigidBodies(Model &model,
                                         bool applyCurrentTransforms) {
  auto &rigidBodies = model.physics.m_impl->rigidBodies;
  rigidBodies.clear();
  rigidBodies.reserve(model.data->rigidBodies.size());
  for (const auto &rigidBody : model.data->rigidBodies) {
    auto &body = rigidBodies.emplace_back();

    switch (rigidBody.shape) {
    case RigidBodyShape::Sphere:
      body.shape = std::make_unique<btSphereShape>(rigidBody.getSphereRadius());
      break;
    case RigidBodyShape::Capsule:
      body.shape = std::make_unique<btCapsuleShape>(
          rigidBody.getCapsuleRadius(), rigidBody.getCapsuleHeight());
      break;
    case RigidBodyShape::Box:
      body.shape = std::make_unique<btBoxShape>(
          glm2btVector3(rigidBody.getBoxHalfExtents()));
      break;
    }

    btVector3 localInertia;
    float mass = rigidBody.physicsCalcType == PhysicsCalcType::Static
                     ? 0.f
                     : rigidBody.mass;
    if (mass != 0.f)
      body.shape->calculateLocalInertia(mass, localInertia);
    else
      localInertia.setZero();

    btTransform offset;

    glm::mat4 rot = glm::eulerAngleYXZ(
        rigidBody.rotation.y, rigidBody.rotation.x, rigidBody.rotation.z);
    body.offset.rotation = glm::quat_cast(rot);
    btMatrix3x3 m;
    m.setIdentity();
    m.setFromOpenGLSubMatrix(&rot[0][0]);
    offset.setBasis(m);
    body.offset.translation = rigidBody.position;
    offset.setOrigin(glm2btVector3(rigidBody.position));

    if (applyCurrentTransforms && rigidBody.boneIndex >= 0) {
      int32_t j = rigidBody.boneIndex;

      Transform t = body.offset;
      t.translation -= model.data->bones[j].position;
      t *= model.pose.globalBoneTransform(j);

      body.motionState =
          std::make_unique<btDefaultMotionState>(glm2btTransform(t));
    } else
      body.motionState = std::make_unique<btDefaultMotionState>(offset);

    btRigidBody::btRigidBodyConstructionInfo info(
        mass, body.motionState.get(), body.shape.get(), localInertia);
    info.m_linearDamping = rigidBody.linearDamping;
    info.m_angularDamping = rigidBody.angularDamping;
    info.m_restitution = rigidBody.restitution;
    info.m_friction = rigidBody.friction;
    info.m_additionalDamping = true;

    body.rigidBody = std::make_unique<btRigidBody>(info);
    body.rigidBody->setSleepingThresholds(0.f, 0.f);

    if (rigidBody.physicsCalcType == PhysicsCalcType::Static) {
      body.rigidBody->setCollisionFlags(body.rigidBody->getCollisionFlags() |
                                        btCollisionObject::CF_KINEMATIC_OBJECT);
      body.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    } else {
      body.rigidBody->setActivationState(ACTIVE_TAG);
    }

    m_impl->world->addRigidBody(body.rigidBody.get(), 1 << rigidBody.group,
                                rigidBody.collisionGroupMask);
  }
}

void PhysicsWorld::setupModelJoints(Model &model) {
  auto &rigidBodies = model.physics.m_impl->rigidBodies;
  auto &joints = model.physics.m_impl->joints;
  joints.clear();
  joints.reserve(model.data->joints.size());
  for (const auto &joint : model.data->joints) {
    if (joint.rigidBodyIndexA < 0 || joint.rigidBodyIndexB < 0 ||
        joint.rigidBodyIndexA == joint.rigidBodyIndexB)
      continue;

    const auto &a = rigidBodies[joint.rigidBodyIndexA];
    const auto &b = rigidBodies[joint.rigidBodyIndexB];

    if (a.rigidBody->getMass() == 0.f && b.rigidBody->getMass() == 0.f)
      continue;

    auto &constraint = joints.emplace_back();

    btTransform transform;
    transform.setOrigin(glm2btVector3(joint.position));
    transform.setBasis(eulerAnglesToMatrix(joint.rotation));

    auto invA = glm2btTransform(a.offset).inverseTimes(transform);
    auto invB = glm2btTransform(b.offset).inverseTimes(transform);

    constraint = std::make_unique<btGeneric6DofSpringConstraint>(
        *a.rigidBody, *b.rigidBody, invA, invB, true);

    constraint->setLinearLowerLimit(glm2btVector3(joint.linearLowerLimit));
    constraint->setLinearUpperLimit(glm2btVector3(joint.linearUpperLimit));
    constraint->setAngularLowerLimit(glm2btVector3(joint.angularLowerLimit));
    constraint->setAngularUpperLimit(glm2btVector3(joint.angularUpperLimit));

    for (int i = 0; i < 3; ++i) {
      constraint->setStiffness(i, joint.linearStiffness[i]);
      constraint->enableSpring(i, !btFuzzyZero(joint.linearStiffness[i]));
      constraint->setStiffness(i + 3, joint.angularStiffness[i]);
      constraint->enableSpring(i + 3, !btFuzzyZero(joint.angularStiffness[i]));
    }

    m_impl->world->addConstraint(constraint.get());
  }
}

void PhysicsWorld::clearModelPhysics(Model &model) {
  auto &impl = *model.physics.m_impl;
  for (const auto &j : impl.joints)
    m_impl->world->removeConstraint(j.get());
  impl.joints.clear();

  for (const auto &r : impl.rigidBodies)
    m_impl->world->removeRigidBody(r.rigidBody.get());
  impl.rigidBodies.clear();
}

} // namespace glmmd

#else // no physics backend

namespace glmmd {

class PhysicsWorldImpl {};

PhysicsWorld::PhysicsWorld() {}
PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld &&) noexcept = default;
PhysicsWorld &PhysicsWorld::operator=(PhysicsWorld &&) noexcept = default;

void PhysicsWorld::update(float, int, float) {}
void PhysicsWorld::setGravity(const glm::vec3 &) {}
void PhysicsWorld::setupModelPhysics(Model &, bool) {}
void PhysicsWorld::clearModelPhysics(Model &) {}
void PhysicsWorld::setupModelRigidBodies(Model &, bool) {}
void PhysicsWorld::setupModelJoints(Model &) {}

} // namespace glmmd

#endif // GLMMD_DONT_USE_BULLET
