#ifndef GLMMD_CORE_SIMPLE_ANIMATOR_H_
#define GLMMD_CORE_SIMPLE_ANIMATOR_H_

#include <memory>

#include <glmmd/core/Motion.h>
#include <glmmd/core/Animator.h>

namespace glmmd
{

class SimpleAnimator : public Animator
{
public:
    SimpleAnimator(const std::shared_ptr<const Motion> &motion)
        : Animator()
        , m_motion(motion)
    {
    }

    virtual ~SimpleAnimator() override = default;

    virtual bool isFinished() const override
    {
        return getElapsedTime() > m_motion->duration();
    }

    virtual void getLocalPose(ModelPose &pose) const override
    {
        m_motion->getLocalPose(getElapsedTime(), pose);
    }

private:
    std::shared_ptr<const Motion> m_motion;
};

} // namespace glmmd

#endif
