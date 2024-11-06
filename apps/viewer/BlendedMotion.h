#ifndef BLENDED_MOTION_H_
#define BLENDED_MOTION_H_

#include <memory>
#include <string>

#include <glmmd/core/Motion.h>

class BlendedMotion : public glmmd::Motion
{
public:
    BlendedMotion(const std::shared_ptr<const glmmd::ModelData> &modelData)
        : glmmd::Motion()
        , m_modelData(modelData)
        , m_duration(0.f)
    {
    }

    void addMotion(const std::string                    &label,
                   const std::shared_ptr<glmmd::Motion> &motion)
    {
        m_labels.push_back(label);
        m_motions.push_back(motion);
        m_duration = std::max(m_duration, motion->duration());
    }

    void removeMotion(size_t i)
    {
        if (i >= m_motions.size())
            return;
        m_labels.erase(m_labels.begin() + i);
        m_motions.erase(m_motions.begin() + i);
        m_duration = 0.f;
        for (const auto &motion : m_motions)
            m_duration = std::max(m_duration, motion->duration());
    }

    bool moveUp(size_t i)
    {
        if (i == 0 || i >= m_motions.size())
            return false;
        std::swap(m_labels[i], m_labels[i - 1]);
        std::swap(m_motions[i], m_motions[i - 1]);
        return true;
    }

    bool moveDown(size_t i)
    {
        if (i == m_motions.size() - 1 || i >= m_motions.size())
            return false;
        std::swap(m_labels[i], m_labels[i + 1]);
        std::swap(m_motions[i], m_motions[i + 1]);
        return true;
    }

    virtual float duration() const override { return m_duration; }

    virtual void getLocalPose(float time, glmmd::ModelPose &pose) const override
    {
        glmmd::ModelPose p(m_modelData);
        for (const auto &motion : m_motions)
        {
            p.resetLocal();
            motion->getLocalPose(time, p);
            pose += p;
        }
    }

    const auto &labels() const { return m_labels; }

    bool   empty() const { return m_motions.empty(); }
    size_t size() const { return m_motions.size(); }

private:
    std::shared_ptr<const glmmd::ModelData>     m_modelData;
    std::vector<std::string>                    m_labels;
    std::vector<std::shared_ptr<glmmd::Motion>> m_motions;

    float m_duration;
};

#endif
