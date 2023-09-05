#include "IdleAnimator.h"

void IdleAnimator::getLocalPose(glmmd::ModelPose &pose) const
{
    auto now = std::chrono::high_resolution_clock::now();

    float time  = getElapsedTime();
    float time2 = std::chrono::duration<float>(now - m_timePoint).count();

    if (m_idleMotions.empty())
    {
        m_baseMotion->getLocalPose(time, pose);
        return;
    }

    if (m_currentMotionIndex >= 0)
    {
        auto &motion = m_idleMotions[m_currentMotionIndex];
        if (time2 > motion->duration())
        {
            m_baseMotion->getLocalPose(time, pose);
            m_currentMotionIndex = -1;
            m_timePoint          = now;
            m_deltaTime          = m_expDist(m_gen);
        }
        else
        {
            if (time2 < m_transitionTime ||
                time2 > motion->duration() - m_transitionTime)
            {
                float t = time2 < m_transitionTime
                              ? time2 / m_transitionTime
                              : (motion->duration() - time2) / m_transitionTime;
                m_baseMotion->getLocalPose(time, pose);
                glmmd::ModelPose pose2(m_modelData);
                motion->getLocalPose(time2, pose2);
                pose.blendWith(pose2, t);
            }
            else
                motion->getLocalPose(time2, pose);
        }
    }
    else
    {
        m_baseMotion->getLocalPose(time, pose);
        if (time2 > m_deltaTime)
        {
            m_currentMotionIndex = m_uniDist(m_gen);
            m_timePoint          = now;
        }
    }
}
