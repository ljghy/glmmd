#ifndef CHAT_VISEME_MOTION_H_
#define CHAT_VISEME_MOTION_H_

#include <glmmd/core/ModelData.h>
#include <glmmd/core/Motion.h>

inline constexpr size_t nVisemeTypes    = 55;
inline constexpr float  VisemeFrameRate = 60.f;

using Viseme = std::array<float, nVisemeTypes>;

class VisemeMotion : public glmmd::Motion
{
public:
    VisemeMotion(const glmmd::ModelData    &data,
                 const std::vector<Viseme> &visemes);

    virtual float duration() const override
    {
        return m_visemes.size() / VisemeFrameRate;
    }

    virtual void getLocalPose(float             time,
                              glmmd::ModelPose &pose) const override;

private:
    std::vector<Viseme>               m_visemes;
    std::array<int32_t, nVisemeTypes> m_morphMap;
};

#endif
