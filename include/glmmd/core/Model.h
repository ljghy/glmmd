#ifndef GLMMD_CORE_MODEL_H_
#define GLMMD_CORE_MODEL_H_

#include <memory>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/ModelPose.h>
#include <glmmd/core/ModelPoseSolver.h>

namespace glmmd
{

class Model
{
public:
    Model(const std::shared_ptr<ModelData> &data)
        : m_data(data)
        , m_pose(data)
        , m_poseSolver(data)
    {
        m_pose.resetLocal();
        m_poseSolver.solveBeforePhysics(m_pose);
        m_poseSolver.solveAfterPhysics(m_pose);
    }

    Model(const Model &other)            = delete;
    Model &operator=(const Model &other) = delete;

    Model(Model &&other) noexcept            = default;
    Model &operator=(Model &&other) noexcept = default;

    ModelData       &data() { return *m_data; }
    const ModelData &data() const { return *m_data; }

    ModelPose       &pose() { return m_pose; }
    const ModelPose &pose() const { return m_pose; }

    ModelPhysics       &physics() { return m_physics; }
    const ModelPhysics &physics() const { return m_physics; }

    void resetLocalPose() { m_pose.resetLocal(); }

    void solvePose()
    {
        m_poseSolver.solveBeforePhysics(m_pose);
        m_poseSolver.syncWithPhysics(m_pose, m_physics);
        m_poseSolver.solveAfterPhysics(m_pose);
    }

private:
    std::shared_ptr<ModelData> m_data;
    ModelPhysics               m_physics;
    ModelPose                  m_pose;
    ModelPoseSolver            m_poseSolver;
};

} // namespace glmmd

#endif