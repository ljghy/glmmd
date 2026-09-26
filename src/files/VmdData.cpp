#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/gtx/euler_angles.hpp>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VmdData.h>

namespace glmmd {

MotionClip VmdData::toMotionClip(const ModelData &modelData, bool loop,
                                 float frameRate) const {
  MotionClip clip(loop, frameRate);

  clip.frameCount = 0;

  std::unordered_map<std::string, uint32_t> boneNameToIndex;
  for (uint32_t i = 0; i < modelData.bones.size(); ++i)
    boneNameToIndex.emplace(modelData.bones[i].name, i);

  // Bucket each keyframe by target bone index, then flatten into a single
  // contiguous array sorted by frame number within each bone's span. eval()
  // binary-searches those spans.
  std::vector<std::vector<MotionClip::BoneKeyFrame>> boneBuckets(
      modelData.bones.size());
  for (const auto &vbf : boneFrames) {
    clip.frameCount = std::max(clip.frameCount, vbf.frameNumber);

    auto it = boneNameToIndex.find(codeCvt<ShiftJIS, UTF8>(vbf.boneName));
    if (it == boneNameToIndex.end())
      continue;

    auto &mbf = boneBuckets[it->second].emplace_back();
    mbf.frameNumber = vbf.frameNumber;
    mbf.transform = {vbf.translation, vbf.rotation};

    for (int i = 0; i < 4; ++i)
      mbf.xCurve[i] = vbf.interpolation[i * 4 + 0] / 127.f;
    for (int i = 0; i < 4; ++i)
      mbf.yCurve[i] = vbf.interpolation[i * 4 + 16] / 127.f;
    for (int i = 0; i < 4; ++i)
      mbf.zCurve[i] = vbf.interpolation[i * 4 + 32] / 127.f;
    for (int i = 0; i < 4; ++i)
      mbf.rCurve[i] = vbf.interpolation[i * 4 + 48] / 127.f;
  }

  clip.boneFrameRange.resize(modelData.bones.size());
  clip.boneFrames.reserve(boneFrames.size());
  for (size_t i = 0; i < boneBuckets.size(); ++i) {
    auto &bucket = boneBuckets[i];
    std::stable_sort(bucket.begin(), bucket.end(),
                     [](const MotionClip::BoneKeyFrame &a,
                        const MotionClip::BoneKeyFrame &b) {
                       return a.frameNumber < b.frameNumber;
                     });
    auto first = static_cast<uint32_t>(clip.boneFrames.size());
    clip.boneFrames.insert(clip.boneFrames.end(), bucket.begin(), bucket.end());
    auto last = static_cast<uint32_t>(clip.boneFrames.size());
    clip.boneFrameRange[i] = {first, last};
  }

  std::unordered_map<std::string, uint32_t> morphNameToIndex;
  for (uint32_t i = 0; i < modelData.morphs.size(); ++i)
    morphNameToIndex.emplace(modelData.morphs[i].name, i);

  std::vector<std::vector<MotionClip::MorphKeyFrame>> morphBuckets(
      modelData.morphs.size());
  for (const auto &vmf : morphFrames) {
    clip.frameCount = std::max(clip.frameCount, vmf.frameNumber);

    auto it = morphNameToIndex.find(codeCvt<ShiftJIS, UTF8>(vmf.morphName));
    if (it == morphNameToIndex.end())
      continue;

    auto &mmf = morphBuckets[it->second].emplace_back();
    mmf.frameNumber = vmf.frameNumber;
    mmf.weight = vmf.weight;
  }

  clip.morphFrameRange.resize(modelData.morphs.size());
  clip.morphFrames.reserve(morphFrames.size());
  for (size_t i = 0; i < morphBuckets.size(); ++i) {
    auto &bucket = morphBuckets[i];
    std::stable_sort(bucket.begin(), bucket.end(),
                     [](const MotionClip::MorphKeyFrame &a,
                        const MotionClip::MorphKeyFrame &b) {
                       return a.frameNumber < b.frameNumber;
                     });
    auto first = static_cast<uint32_t>(clip.morphFrames.size());
    clip.morphFrames.insert(clip.morphFrames.end(), bucket.begin(),
                            bucket.end());
    auto last = static_cast<uint32_t>(clip.morphFrames.size());
    clip.morphFrameRange[i] = {first, last};
  }
  return clip;
}

CameraMotion VmdData::toCameraMotion(bool loop, float frameRate) const {
  CameraMotion motion(loop, frameRate);

  motion.frameCount = 1;
  std::unordered_set<uint32_t> seenFrames;
  for (const auto &frame : cameraFrames) {
    if (!seenFrames.insert(frame.frameNumber).second)
      continue;

    motion.frameCount = std::max(motion.frameCount, frame.frameNumber);

    auto &ckf = motion.keyFrames.emplace_back();

    ckf.frameNumber = frame.frameNumber;
    ckf.distance = -frame.distance;
    ckf.target = frame.target;
    ckf.rotation = glm::quat_cast(glm::eulerAngleYZX(
        -frame.rotation.y, -frame.rotation.z, -frame.rotation.x));
    ckf.fov = glm::radians(static_cast<float>(frame.fov));
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

  std::stable_sort(motion.keyFrames.begin(), motion.keyFrames.end(),
                   [](const CameraMotion::CameraKeyFrame &a,
                      const CameraMotion::CameraKeyFrame &b) {
                     return a.frameNumber < b.frameNumber;
                   });

  return motion;
}

} // namespace glmmd