#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VmdData.h>

namespace glmmd
{

FixedMotionClip VmdData::toFixedMotionClip(const ModelData &modelData,
                                           bool loop, float frameRate) const
{
    FixedMotionClip clip(loop, frameRate);

    clip.frameCount = 0;

    std::unordered_map<std::string, uint32_t> boneNameToIndex;
    for (uint32_t i = 0; i < modelData.bones.size(); ++i)
        boneNameToIndex.emplace(modelData.bones[i].name, i);

    clip.boneFrames.reserve(boneFrames.size());
    clip.boneFrameIndex.resize(modelData.bones.size());
    for (const auto &vbf : boneFrames)
    {
        clip.frameCount = std::max(clip.frameCount, vbf.frameNumber);

        auto it = boneNameToIndex.find(codeCvt<ShiftJIS, UTF8>(vbf.boneName));
        if (it == boneNameToIndex.end())
            continue;
        auto boneIndex = it->second;

        auto &mbf     = clip.boneFrames.emplace_back();
        mbf.transform = {vbf.translation, vbf.rotation};

        for (int i = 0; i < 4; ++i)
            mbf.xCurve[i] = vbf.interpolation[i * 4 + 0] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.yCurve[i] = vbf.interpolation[i * 4 + 16] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.zCurve[i] = vbf.interpolation[i * 4 + 32] / 127.f;
        for (int i = 0; i < 4; ++i)
            mbf.rCurve[i] = vbf.interpolation[i * 4 + 48] / 127.f;

        clip.boneFrameIndex[boneIndex][vbf.frameNumber] =
            static_cast<uint32_t>(clip.boneFrames.size() - 1);
    }

    std::unordered_map<std::string, uint32_t> morphNameToIndex;
    for (uint32_t i = 0; i < modelData.morphs.size(); ++i)
        morphNameToIndex.emplace(modelData.morphs[i].name, i);

    clip.morphFrames.reserve(morphFrames.size());
    clip.morphFrameIndex.resize(modelData.morphs.size());
    for (const auto &vmf : morphFrames)
    {
        clip.frameCount = std::max(clip.frameCount, vmf.frameNumber);

        auto it = morphNameToIndex.find(codeCvt<ShiftJIS, UTF8>(vmf.morphName));
        if (it == morphNameToIndex.end())
            continue;
        auto morphIndex = it->second;

        auto &mmf = clip.morphFrames.emplace_back();
        mmf.ratio = vmf.ratio;
        clip.morphFrameIndex[morphIndex][vmf.frameNumber] =
            static_cast<uint32_t>(clip.morphFrames.size() - 1);
    }
    return clip;
}

CameraMotion VmdData::toCameraMotion(bool loop, float frameRate) const
{
    CameraMotion motion(loop, frameRate);

    motion.frameCount = 1;
    for (const auto &frame : cameraFrames)
    {
        auto [iter, inserted] = motion.frameIndex.emplace(
            frame.frameNumber, static_cast<uint32_t>(motion.keyFrames.size()));

        if (!inserted)
            continue;

        motion.frameCount = std::max(motion.frameCount, frame.frameNumber);

        auto &ckf = motion.keyFrames.emplace_back();

        ckf.distance    = -frame.distance;
        ckf.target      = frame.target;
        ckf.rotation    = glm::quat_cast(glm::eulerAngleYZX(
               -frame.rotation.y, -frame.rotation.z, -frame.rotation.x));
        ckf.fov         = glm::radians(static_cast<float>(frame.fov));
        ckf.perspective = frame.perspective == 0u;

        for (int i = 0; i < 4; ++i)
            ckf.distanceCurve[i] = frame.interpolation[i] / 127.f;
        for (int i = 0; i < 4; ++i)
            ckf.targetXCurve[i] = frame.interpolation[i + 4] / 127.f;
        for (int i = 0; i < 4; ++i)
            ckf.targetYCurve[i] = frame.interpolation[i + 8] / 127.f;
        for (int i = 0; i < 4; ++i)
            ckf.targetZCurve[i] = frame.interpolation[i + 12] / 127.f;
        for (int i = 0; i < 4; ++i)
            ckf.rotationCurve[i] = frame.interpolation[i + 16] / 127.f;
        for (int i = 0; i < 4; ++i)
            ckf.fovCurve[i] = frame.interpolation[i + 20] / 127.f;
    }

    return motion;
}

} // namespace glmmd