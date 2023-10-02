#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VmdData.h>

namespace glmmd
{

FixedMotionClip VmdData::toFixedMotionClip(const ModelData &modelData,
                                           bool loop, float frameRate) const
{
    FixedMotionClip clip(loop, frameRate);

    clip.m_frameCount = 0;

    clip.m_boneFrames.reserve(boneFrames.size());
    clip.m_boneFrameIndex.resize(modelData.bones.size());
    for (const auto &vbf : boneFrames)
    {
        clip.m_frameCount = std::max(clip.m_frameCount, vbf.frameNumber);

        auto boneIndex = modelData.getBoneIndex(shiftJIS_to_UTF8(vbf.boneName));
        if (boneIndex == -1)
            continue;

        auto &mbf       = clip.m_boneFrames.emplace_back();
        mbf.translation = vbf.translation;
        mbf.rotation    = vbf.rotation;

        for (int i = 0; i < 4; ++i)
            mbf.xCurve[i] = vbf.interpolation[i * 4 + 0] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.yCurve[i] = vbf.interpolation[i * 4 + 16] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.zCurve[i] = vbf.interpolation[i * 4 + 32] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.rCurve[i] = vbf.interpolation[i * 4 + 48] / 127.f;

        clip.m_boneFrameIndex[boneIndex][vbf.frameNumber] =
            static_cast<uint32_t>(clip.m_boneFrames.size() - 1);
    }

    clip.m_morphFrames.reserve(morphFrames.size());
    clip.m_morphFrameIndex.resize(modelData.morphs.size());
    for (const auto &vmf : morphFrames)
    {
        clip.m_frameCount = std::max(clip.m_frameCount, vmf.frameNumber);

        auto morphIndex =
            modelData.getMorphIndex(shiftJIS_to_UTF8(vmf.morphName));
        if (morphIndex == -1)
            continue;

        auto &mmf = clip.m_morphFrames.emplace_back();
        mmf.ratio = vmf.ratio;
        clip.m_morphFrameIndex[morphIndex][vmf.frameNumber] =
            static_cast<uint32_t>(clip.m_morphFrames.size() - 1);
    }
    return clip;
}

} // namespace glmmd