#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include <glmmd/core/PhysicsWorld.h>

namespace glmmd
{

constexpr float GRAVITY_SCALE = 12.5f;

inline static btVector3 glm2btVector3(const glm::vec3 &v)
{
    return btVector3(v.x, v.y, v.z);
}

inline static btQuaternion glm2btQuaternion(const glm::quat &q)
{
    return btQuaternion(q.x, q.y, q.z, q.w);
}

inline static btTransform glm2btTransform(const glm::vec3 &translation,
                                          const glm::quat &rotation)
{
    btTransform t;
    t.setOrigin(glm2btVector3(translation));
    t.setRotation(glm2btQuaternion(rotation));
    return t;
}

inline static btMatrix3x3 eulerAnglesToMatrix(const glm::vec3 &eulerAngles)
{
    glm::mat4 rot =
        glm::eulerAngleYXZ(eulerAngles.y, eulerAngles.x, eulerAngles.z);
    btMatrix3x3 m;
    m.setIdentity();
    m.setFromOpenGLSubMatrix(&rot[0][0]);
    return m;
}

PhysicsWorld::PhysicsWorld()
    : m_collisionConfig(new btDefaultCollisionConfiguration())
    , m_dispatcher(new btCollisionDispatcher(m_collisionConfig.get()))
    , m_broadphase(new btDbvtBroadphase())
    , m_solver(new btSequentialImpulseConstraintSolver())
    , m_world(new btDiscreteDynamicsWorld(m_dispatcher.get(),
                                          m_broadphase.get(), m_solver.get(),
                                          m_collisionConfig.get()))
    , m_groundShape(new btStaticPlaneShape(btVector3(0.f, 1.f, 0.f), 0.f))
    , m_gravity(btVector3(0.f, -9.8f, 0.f) * GRAVITY_SCALE)
{
    m_world->setGravity(m_gravity);

    btTransform groundTransform;
    groundTransform.setIdentity();
    m_groundMotionState =
        std::make_unique<btDefaultMotionState>(groundTransform);

    btRigidBody::btRigidBodyConstructionInfo groundInfo(
        0.f, m_groundMotionState.get(), m_groundShape.get(),
        btVector3(0.f, 0.f, 0.f));
    m_groundRigidBody = std::make_unique<btRigidBody>(groundInfo);

    m_world->addRigidBody(m_groundRigidBody.get(), 1 << 15, 0x7FFF);
}

void PhysicsWorld::update(float deltaTime, int maxSubSteps,
                          float fixedDeltaTime)
{
    m_world->stepSimulation(deltaTime, maxSubSteps, fixedDeltaTime);
}

void PhysicsWorld::setGravity(const glm::vec3 &gravity)
{
    m_gravity = glm2btVector3(gravity) * GRAVITY_SCALE;
    m_world->setGravity(m_gravity);
}

void PhysicsWorld::setupModelPhysics(Model &model, bool applyCurrentTransforms)
{
    setupModelRigidBodies(model, applyCurrentTransforms);
    setupModelJoints(model);
}

void PhysicsWorld::setupModelRigidBodies(Model &model,
                                         bool   applyCurrentTransforms)
{
    model.physics().rigidBodies.clear();
    model.physics().rigidBodies.reserve(model.data().rigidBodies.size());
    for (const auto &rigidBody : model.data().rigidBodies)
    {
        auto &body = model.physics().rigidBodies.emplace_back();

        switch (rigidBody.shape)
        {
        case RigidBodyShape::Sphere:
            body.shape = std::make_unique<btSphereShape>(rigidBody.size.x);
            break;
        case RigidBodyShape::Capsule:
            body.shape = std::make_unique<btCapsuleShape>(rigidBody.size.x,
                                                          rigidBody.size.y);
            break;
        case RigidBodyShape::Box:
            body.shape =
                std::make_unique<btBoxShape>(glm2btVector3(rigidBody.size));
            break;
        }

        btVector3 localInertia;
        float     mass = rigidBody.physicsCalcType == PhysicsCalcType::Static
                             ? 0.f
                             : rigidBody.mass;
        if (mass != 0.f)
            body.shape->calculateLocalInertia(mass, localInertia);
        else
            localInertia.setZero();

        btTransform offset;

        glm::mat4 rot = glm::eulerAngleYXZ(
            rigidBody.rotation.y, rigidBody.rotation.x, rigidBody.rotation.z);
        body.rotationOffset = glm::quat_cast(rot);
        btMatrix3x3 m;
        m.setIdentity();
        m.setFromOpenGLSubMatrix(&rot[0][0]);
        offset.setBasis(m);
        body.translationOffset = rigidBody.position;
        offset.setOrigin(glm2btVector3(rigidBody.position));

        if (applyCurrentTransforms && rigidBody.boneIndex >= 0)
        {
            int32_t j = rigidBody.boneIndex;

            glm::vec3 translation =
                glm::vec3(model.pose().getGlobalBoneTransform(j) *
                          glm::translate(glm::mat4(1.f),
                                         -model.data().bones[j].position) *
                          glm::vec4(body.translationOffset, 1.f));

            glm::quat rotation =
                glm::quat_cast(model.pose().getGlobalBoneTransform(j)) *
                body.rotationOffset;

            auto transform = glm2btTransform(translation, rotation);
            body.motionState =
                std::make_unique<btDefaultMotionState>(transform);
        }
        else
            body.motionState = std::make_unique<btDefaultMotionState>(offset);

        btRigidBody::btRigidBodyConstructionInfo info(
            mass, body.motionState.get(), body.shape.get(), localInertia);
        info.m_linearDamping     = rigidBody.linearDamping;
        info.m_angularDamping    = rigidBody.angularDamping;
        info.m_restitution       = rigidBody.restitution;
        info.m_friction          = rigidBody.friction;
        info.m_additionalDamping = true;

        body.rigidBody = std::make_unique<btRigidBody>(info);
        body.rigidBody->setSleepingThresholds(0.f, 0.f);

        if (rigidBody.physicsCalcType == PhysicsCalcType::Static)
        {
            body.rigidBody->setCollisionFlags(
                body.rigidBody->getCollisionFlags() |
                btCollisionObject::CF_KINEMATIC_OBJECT);
            body.rigidBody->setActivationState(DISABLE_DEACTIVATION);
        }
        else
        {
            body.rigidBody->setActivationState(ACTIVE_TAG);
        }

        m_world->addRigidBody(body.rigidBody.get(), 1 << rigidBody.group,
                              rigidBody.collisionGroupMask);
    }
}

void PhysicsWorld::setupModelJoints(Model &model)
{
    model.physics().joints.clear();
    model.physics().joints.reserve(model.data().joints.size());
    for (const auto &joint : model.data().joints)
    {
        if (joint.rigidBodyIndexA < 0 || joint.rigidBodyIndexB < 0 ||
            joint.rigidBodyIndexA == joint.rigidBodyIndexB)
            continue;

        const auto &a = model.physics().rigidBodies[joint.rigidBodyIndexA];
        const auto &b = model.physics().rigidBodies[joint.rigidBodyIndexB];

        if (a.rigidBody->getMass() == 0.f && b.rigidBody->getMass() == 0.f)
            continue;

        auto &constraint = model.physics().joints.emplace_back();

        btTransform transform;
        transform.setOrigin(glm2btVector3(joint.position));
        transform.setBasis(eulerAnglesToMatrix(joint.rotation));

        auto invA = glm2btTransform(a.translationOffset, a.rotationOffset)
                        .inverseTimes(transform);
        auto invB = glm2btTransform(b.translationOffset, b.rotationOffset)
                        .inverseTimes(transform);

        constraint = std::make_unique<btGeneric6DofSpringConstraint>(
            *a.rigidBody, *b.rigidBody, invA, invB, true);

        constraint->setLinearLowerLimit(glm2btVector3(joint.linearLowerLimit));
        constraint->setLinearUpperLimit(glm2btVector3(joint.linearUpperLimit));
        constraint->setAngularLowerLimit(
            glm2btVector3(joint.angularLowerLimit));
        constraint->setAngularUpperLimit(
            glm2btVector3(joint.angularUpperLimit));

        for (int i = 0; i < 3; ++i)
        {
            constraint->setStiffness(i, joint.linearStiffness[i]);
            constraint->enableSpring(i, !btFuzzyZero(joint.linearStiffness[i]));
            constraint->setStiffness(i + 3, joint.angularStiffness[i]);
            constraint->enableSpring(i + 3,
                                     !btFuzzyZero(joint.angularStiffness[i]));
        }

        m_world->addConstraint(constraint.get());
    }
}

void PhysicsWorld::clearModelPhysics(Model &model)
{
    for (const auto &j : model.physics().joints)
        m_world->removeConstraint(j.get());
    model.physics().joints.clear();

    for (const auto &r : model.physics().rigidBodies)
        m_world->removeRigidBody(r.rigidBody.get());
    model.physics().rigidBodies.clear();
}

} // namespace glmmd