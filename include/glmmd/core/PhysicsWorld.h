#ifndef GLMMD_CORE_PHYSICS_WORLD_H_
#define GLMMD_CORE_PHYSICS_WORLD_H_

#include <memory>

#include <glmmd/core/Model.h>

namespace glmmd
{

class PhysicsWorld
{
public:
    PhysicsWorld();

    void setupModelPhysics(Model &model, bool applyCurrentTransforms = false);

    void update(float deltaTime, int maxSubSteps = 1,
                float fixedDeltaTime = 1.f / 60.f);

    void setGravity(const glm::vec3 &gravity);

private:
    void setupModelRigidBodies(Model &model, bool applyCurrentTransforms);
    void setupModelJoints(Model &model, bool applyCurrentTransforms);

private:
    std::unique_ptr<btDefaultCollisionConfiguration>     m_collisionConfig;
    std::unique_ptr<btCollisionDispatcher>               m_dispatcher;
    std::unique_ptr<btBroadphaseInterface>               m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld>             m_world;

    std::unique_ptr<btCollisionShape>     m_groundShape;
    std::unique_ptr<btDefaultMotionState> m_groundMotionState;
    std::unique_ptr<btRigidBody>          m_groundRigidBody;

    btVector3 m_gravity;
};

} // namespace glmmd

#endif