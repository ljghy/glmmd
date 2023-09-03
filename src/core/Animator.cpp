#include <glmmd/core/Animator.h>

namespace glmmd
{

Animator::Animator(const AnimatorState &initialState)
    : m_currentState(initialState)
{
}

void Animator::registerMotion(std::unique_ptr<Motion> &&motion)
{
    m_states.emplace_back(std::move(motion));
    m_transitionTable.emplace_back();
}

void Animator::registerTransition(const MotionTransition &transition)
{
    m_transitions.emplace_back(transition);
    m_transitionTable[transition.fromStateIndex].emplace_back(
        static_cast<int32_t>(m_transitions.size() - 1));
}

void Animator::update(float currentTime)
{
    if (m_states.empty())
        return;

    if (m_currentState.onTransition)
    {
        float transitionTime = currentTime - m_currentState.stateStartTime;
        if (transitionTime >=
            m_transitions[m_currentState.index].transitionTime)
        {
            m_currentState.motionStartTime = m_currentState.stateStartTime =
                currentTime;
            m_currentState.onTransition = false;
            m_currentState.index =
                m_transitions[m_currentState.index].toStateIndex;
        }
    }
    else
    {
        for (int32_t i : m_transitionTable[m_currentState.index])
        {
            if (m_transitions[i].transitionCondition(
                    *m_states[m_currentState.index],
                    currentTime - m_currentState.motionStartTime))
            {
                m_currentState.stateStartTime = currentTime;
                m_currentState.onTransition   = true;
                m_currentState.index          = i;
                break;
            }
        }
    }
}

void Animator::getLocalPose(float currentTime, ModelPose &pose) const
{
    if (m_states.empty())
        return;

    int32_t i = m_currentState.index;
    if (m_currentState.onTransition)
    {
        m_states[m_transitions[i].fromStateIndex]->getLocalPose(
            currentTime - m_currentState.motionStartTime, pose);
        float transitionTime = currentTime - m_currentState.stateStartTime;
        if (transitionTime != 0.f)
        {
            ModelPose other(pose);
            m_states[m_transitions[i].toStateIndex]->getLocalPose(
                transitionTime, other);

            float t = m_transitions[i].transitionCurve(
                transitionTime / m_transitions[i].transitionTime);

            pose.blendWith(other, t);
        }
    }
    else
    {
        m_states[i]->getLocalPose(currentTime - m_currentState.motionStartTime,
                                  pose);
    }
}

} // namespace glmmd