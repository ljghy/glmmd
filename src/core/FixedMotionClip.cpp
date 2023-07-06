#include <glmmd/core/FixedMotionClip.h>

namespace glmmd
{

FixedMotionClip::FixedMotionClip(bool loop, float frameRate)
    : m_loop(loop)
    , m_frameRate(frameRate)
    , m_frameCount(1)
{
}

float FixedMotionClip::evalCurve(const InterpolationCurve &curve, float x)
{
    const float &x1 = curve[0], &y1 = curve[1], &x2 = curve[2], &y2 = curve[3];

    float t = x, a = 3 * (x1 - x2) + 1, b = 2 * x2 - 4 * x1;
    float t2, s, f, df;
    for (int i = 0; i < 4; ++i)
    {
        t2 = t * t;
        s  = 1 - t;
        df = a * t2 + b * t + 1;

        if (df == 0)
            break;

        f = s * t * (s * x1 + t * x2) + (t2 * t - x) / 3;

        float delta = f / df;
        t -= delta;
        if (std::abs(delta) < 1e-5f)
            break;
    }
    return 3 * s * t * (s * y1 + t * y2) + t2 * t;
}

void FixedMotionClip::getPoseLocal(float time, ModelPose &pose) const
{
    constexpr glm::quat identityRot(1.f, 0.f, 0.f, 0.f);

    if (m_frameCount == 0)
        return;

    float frameTime = m_frameRate * time;
    if (m_loop)
        frameTime = std::fmod(frameTime, static_cast<float>(m_frameCount));
    else
        frameTime = glm::clamp(frameTime, 0.f, 0.9999f * m_frameCount);

    uint32_t frameNumber = static_cast<int32_t>(frameTime);

    for (uint32_t i = 0; i < m_boneFrameIndex.size(); ++i)
    {
        const auto &boneFrameMap = m_boneFrameIndex[i];
        if (boneFrameMap.empty())
        {
            pose.setLocalBoneTranslation(i, glm::vec3(0.f));
            pose.setLocalBoneRotation(i, identityRot);
            continue;
        }

        float t;
        auto  iter = boneFrameMap.upper_bound(frameNumber);
        if (iter == boneFrameMap.begin())
        {
            const auto &frame = m_boneFrames[iter->second];

            t = frameTime / iter->first;
            glm::vec3 tt{evalCurve(frame.xCurve, t), evalCurve(frame.yCurve, t),
                         evalCurve(frame.zCurve, t)};
            float     tr = evalCurve(frame.rCurve, t);

            pose.setLocalBoneTranslation(i, tt * frame.translation);
            pose.setLocalBoneRotation(
                i, glm::slerp(identityRot, frame.rotation, tr));
        }
        else if (iter == boneFrameMap.end())
        {
            const auto &frame = m_boneFrames[boneFrameMap.rbegin()->second];

            t = (frameTime - boneFrameMap.rbegin()->first) /
                (m_frameCount - boneFrameMap.rbegin()->first);
            pose.setLocalBoneTranslation(i, frame.translation);
            pose.setLocalBoneRotation(i, frame.rotation);
        }
        else
        {
            auto        succ       = iter--;
            const auto &leftFrame  = m_boneFrames[iter->second],
                       &rightFrame = m_boneFrames[succ->second];

            t = (frameTime - iter->first) / (succ->first - iter->first);
            glm::vec3 tt{evalCurve(leftFrame.xCurve, t),
                         evalCurve(leftFrame.yCurve, t),
                         evalCurve(leftFrame.zCurve, t)};
            float     tr = evalCurve(leftFrame.rCurve, t);

            pose.setLocalBoneTranslation(i, (1.f - tt) * leftFrame.translation +
                                                tt * rightFrame.translation);
            pose.setLocalBoneRotation(
                i, glm::slerp(leftFrame.rotation, rightFrame.rotation, tr));
        }
    }

    for (uint32_t i = 0; i < m_morphFrameIndex.size(); ++i)
    {
        const auto &morphFrameMap = m_morphFrameIndex[i];
        if (morphFrameMap.empty())
        {
            pose.setMorphRatio(i, 0.f);
            continue;
        }

        float t;
        auto  iter = morphFrameMap.upper_bound(frameNumber);

        if (iter == morphFrameMap.begin())
        {
            t = frameTime / iter->first;
            pose.setMorphRatio(i, t * m_morphFrames[iter->second].ratio);
        }
        else if (iter == morphFrameMap.end())
        {
            t = (frameTime - morphFrameMap.rbegin()->first) /
                (m_frameCount - morphFrameMap.rbegin()->first);
            pose.setMorphRatio(
                i, (1.f - t) *
                       m_morphFrames[morphFrameMap.rbegin()->second].ratio);
        }
        else
        {
            auto        succ       = iter--;
            const auto &leftFrame  = m_morphFrames[iter->second],
                       &rightFrame = m_morphFrames[succ->second];
            t = (frameTime - iter->first) / (succ->first - iter->first);
            pose.setMorphRatio(i,
                               glm::mix(leftFrame.ratio, rightFrame.ratio, t));
        }
    }
}

} // namespace glmmd