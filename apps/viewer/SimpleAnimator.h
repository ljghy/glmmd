#ifndef SIMPLE_ANIMATOR_H_
#define SIMPLE_ANIMATOR_H_

#include <memory>

#include <glmmd/core/Motion.h>
#include <glmmd/core/Animator.h>

class SimpleAnimator : public glmmd::Animator
{
public:
    SimpleAnimator()
        : glmmd::Animator()
        , m_duration(0.f)
    {
    }

    virtual ~SimpleAnimator() override = default;

    void addMotion(const std::shared_ptr<glmmd::Motion> &motion)
    {
        m_motions.push_back(motion);
        m_duration = std::max(m_duration, motion->duration());
    }

    virtual bool isFinished() const override
    {
        return getElapsedTime() > m_duration;
    }

    virtual void getLocalPose(glmmd::ModelPose &pose) const override
    {
        for (const auto &motion : m_motions)
        {
            glmmd::ModelPose p(pose);
            p.resetLocal();
            motion->getLocalPose(getElapsedTime(), p);
            pose += p;
        }
    }

private:
    std::vector<std::shared_ptr<glmmd::Motion>> m_motions;

    float m_duration;
};

#endif
