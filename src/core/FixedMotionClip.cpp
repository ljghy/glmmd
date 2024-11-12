#include <cmath>

#include <glmmd/core/FixedMotionClip.h>

namespace glmmd
{

FixedMotionClip::FixedMotionClip(bool loop_, float frameRate_)
    : loop(loop_)
    , frameRate(frameRate_)
    , frameCount(1)
{
}

void FixedMotionClip::getLocalPose(float time, ModelPose &pose) const
{
    if (frameCount == 0)
        return;

    float frameTime = frameRate * time;
    if (loop)
        frameTime = std::fmod(frameTime, static_cast<float>(frameCount));
    else
        frameTime = glm::clamp(frameTime, 0.f, 0.9999f * frameCount);

    uint32_t frameNumber = static_cast<int32_t>(frameTime);

    for (uint32_t i = 0; i < boneFrameIndex.size(); ++i)
    {
        const auto &boneFrameMap = boneFrameIndex[i];
        if (boneFrameMap.empty())
        {
            pose.setLocalBoneTransform(i, Transform::identity);
            continue;
        }

        auto iter = boneFrameMap.upper_bound(frameNumber);
        if (iter == boneFrameMap.begin())
        {
            const auto &frame = boneFrames[iter->second];

            float     t = frameTime / iter->first;
            glm::vec3 tt{evalCurve(frame.xCurve, t), evalCurve(frame.yCurve, t),
                         evalCurve(frame.zCurve, t)};
            float     tr = evalCurve(frame.rCurve, t);

            pose.setLocalBoneTranslation(i, tt * frame.transform.translation);
            pose.setLocalBoneRotation(i,
                                      glm::slerp(glm::identity<glm::quat>(),
                                                 frame.transform.rotation, tr));
        }
        else if (iter == boneFrameMap.end())
        {
            const auto &frame = boneFrames[boneFrameMap.rbegin()->second];
            pose.setLocalBoneTransform(i, frame.transform);
        }
        else
        {
            auto        succ       = iter--;
            const auto &leftFrame  = boneFrames[iter->second],
                       &rightFrame = boneFrames[succ->second];

            float t = (frameTime - iter->first) / (succ->first - iter->first);
            glm::vec3 tt{evalCurve(leftFrame.xCurve, t),
                         evalCurve(leftFrame.yCurve, t),
                         evalCurve(leftFrame.zCurve, t)};
            float     tr = evalCurve(leftFrame.rCurve, t);

            pose.setLocalBoneTranslation(
                i, (1.f - tt) * leftFrame.transform.translation +
                       tt * rightFrame.transform.translation);
            pose.setLocalBoneRotation(
                i, glm::slerp(leftFrame.transform.rotation,
                              rightFrame.transform.rotation, tr));
        }
    }

    for (uint32_t i = 0; i < morphFrameIndex.size(); ++i)
    {
        const auto &morphFrameMap = morphFrameIndex[i];
        if (morphFrameMap.empty())
        {
            pose.setMorphRatio(i, 0.f);
            continue;
        }

        auto iter = morphFrameMap.upper_bound(frameNumber);

        if (iter == morphFrameMap.begin())
        {
            float t = frameTime / iter->first;
            pose.setMorphRatio(i, t * morphFrames[iter->second].ratio);
        }
        else if (iter == morphFrameMap.end())
        {
            pose.setMorphRatio(
                i, morphFrames[morphFrameMap.rbegin()->second].ratio);
        }
        else
        {
            auto        succ       = iter--;
            const auto &leftFrame  = morphFrames[iter->second],
                       &rightFrame = morphFrames[succ->second];
            float t = (frameTime - iter->first) / (succ->first - iter->first);
            pose.setMorphRatio(i,
                               glm::mix(leftFrame.ratio, rightFrame.ratio, t));
        }
    }
}

} // namespace glmmd