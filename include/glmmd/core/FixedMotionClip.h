#ifndef GLMMD_CORE_FIXED_MOTION_CLIP_H_
#define GLMMD_CORE_FIXED_MOTION_CLIP_H_

#include <map>
#include <vector>

#include <glmmd/core/InterpolationCurve.h>
#include <glmmd/core/Motion.h>
#include <glmmd/core/Transform.h>

namespace glmmd
{

struct FixedMotionClip : public Motion
{
    struct BoneKeyFrame
    {
        Transform transform;

        InterpolationCurvePoints xCurve;
        InterpolationCurvePoints yCurve;
        InterpolationCurvePoints zCurve;
        InterpolationCurvePoints rCurve;
    };

    struct MorphKeyFrame
    {
        float ratio;
    };

    FixedMotionClip(bool loop = false, float frameRate = 30.f);

    float duration() const override { return frameCount / frameRate; }

    void getLocalPose(float time, ModelPose &pose) const override;

    bool  loop;
    float frameRate;

    uint32_t frameCount;

    std::vector<std::map<uint32_t, uint32_t>> boneFrameIndex;
    std::vector<std::map<uint32_t, uint32_t>> morphFrameIndex;

    std::vector<BoneKeyFrame>  boneFrames;
    std::vector<MorphKeyFrame> morphFrames;
};

} // namespace glmmd

#endif
