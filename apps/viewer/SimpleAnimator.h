#ifndef SIMPLE_ANIMATOR_H_
#define SIMPLE_ANIMATOR_H_

#include <memory>

#include <glmmd/core/Motion.h>
#include <glmmd/core/Animator.h>

class SimpleAnimator : public glmmd::Animator
{
public:
    SimpleAnimator(const std::shared_ptr<const glmmd::ModelData> &modelData)
        : glmmd::Animator()
        , m_modelData(modelData)
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
        auto t = getElapsedTime();

        glmmd::ModelPose p(m_modelData);
        for (const auto &motion : m_motions)
        {
            p.resetLocal();
            motion->getLocalPose(t, p);
            pose += p;
        }
    }

private:
    std::shared_ptr<const glmmd::ModelData> m_modelData;

    std::vector<std::shared_ptr<glmmd::Motion>> m_motions;

    float m_duration;
};

#endif
