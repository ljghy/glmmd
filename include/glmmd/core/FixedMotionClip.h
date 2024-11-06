#ifndef GLMMD_CORE_FIXED_MOTION_CLIP_H_
#define GLMMD_CORE_FIXED_MOTION_CLIP_H_

#include <vector>
#include <map>

#include <glmmd/core/Motion.h>
#include <glmmd/core/Transform.h>

namespace glmmd
{

class FixedMotionClip : public Motion
{
    friend struct VmdData;

private:
    using InterpolationCurve = glm::vec4;

    struct BoneKeyFrame
    {
        Transform transform;

        InterpolationCurve xCurve;
        InterpolationCurve yCurve;
        InterpolationCurve zCurve;
        InterpolationCurve rCurve;
    };

    struct MorphKeyFrame
    {
        float ratio;
    };

public:
    FixedMotionClip(bool loop = false, float frameRate = 30.f);

    virtual float duration() const override
    {
        return m_frameCount / m_frameRate;
    }

    virtual void getLocalPose(float time, ModelPose &pose) const override;

public:
    static float evalCurve(const InterpolationCurve &curve, float x);

private:
    bool  m_loop;
    float m_frameRate;

    uint32_t m_frameCount;

    std::vector<std::map<uint32_t, uint32_t>> m_boneFrameIndex;
    std::vector<std::map<uint32_t, uint32_t>> m_morphFrameIndex;

    std::vector<BoneKeyFrame>  m_boneFrames;
    std::vector<MorphKeyFrame> m_morphFrames;
};

} // namespace glmmd

#endif
