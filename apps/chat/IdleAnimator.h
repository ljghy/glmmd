#ifndef CHAT_IDLE_ANIMATOR_H_
#define CHAT_IDLE_ANIMATOR_H_

#include <memory>
#include <random>

#include <glmmd/core/Animator.h>
#include <glmmd/core/Motion.h>

class IdleAnimator : public glmmd::Animator
{
public:
    IdleAnimator(
        const std::shared_ptr<const glmmd::ModelData>           &modelData,
        const std::shared_ptr<const glmmd::Motion>              &baseMotion,
        const std::vector<std::shared_ptr<const glmmd::Motion>> &idleMotions,
        float interval, float transitionTime)
        : glmmd::Animator()
        , m_modelData(modelData)
        , m_baseMotion(baseMotion)
        , m_idleMotions(idleMotions)
        , m_transitionTime(transitionTime)
        , m_currentMotionIndex(-1)
        , m_gen(std::random_device{}())
        , m_expDist(1.f / interval)
        , m_uniDist(0, int(idleMotions.size()) - 1)
        , m_timePoint(m_startTime)
        , m_deltaTime(m_expDist(m_gen))
    {
    }

    virtual ~IdleAnimator() override = default;

    virtual bool isFinished() const override { return false; }
    virtual void getLocalPose(glmmd::ModelPose &pose) const override;

private:
    std::shared_ptr<const glmmd::ModelData>           m_modelData;
    std::shared_ptr<const glmmd::Motion>              m_baseMotion;
    std::vector<std::shared_ptr<const glmmd::Motion>> m_idleMotions;

    float m_transitionTime;

    mutable std::mt19937                         m_gen;
    mutable std::exponential_distribution<float> m_expDist;
    mutable std::uniform_int_distribution<int>   m_uniDist;

    mutable int m_currentMotionIndex;
    mutable std::chrono::time_point<std::chrono::high_resolution_clock>
                  m_timePoint;
    mutable float m_deltaTime;
};

#endif
