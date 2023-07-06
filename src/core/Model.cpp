#include <glmmd/core/Model.h>

namespace glmmd
{

void Model::update(float currentTime)
{
    resetLocalPose();
    updateAnimators(currentTime);
    m_poseSolver.solveBeforePhysics(m_pose);
    m_poseSolver.syncWithPhysics(m_pose, m_physics);
    m_poseSolver.solveAfterPhysics(m_pose);
}

void Model::resetLocalPose() { m_pose.resetLocal(); }

void Model::updateAnimators(float currentTime)
{
    int i = 0;
    for (auto &animator : m_animators)
    {
        animator->update(currentTime);
        if (i++ == 0)
            animator->getPoseLocal(currentTime, m_pose);
        else
        {
            ModelPose pose(m_pose);
            animator->getPoseLocal(currentTime, pose);
            m_pose += pose;
        }
    }
}

} // namespace glmmd