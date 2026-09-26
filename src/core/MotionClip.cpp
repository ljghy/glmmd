#include <glmmd/core/MotionClip.h>

#include <algorithm>
#include <cmath>

namespace glmmd {

MotionClip::MotionClip(bool loop_, float frameRate_)
    : loop(loop_), frameRate(frameRate_), frameCount(1) {}

void MotionClip::eval(float time, Pose &pose) const {
  if (frameCount == 0)
    return;

  float frameTime = frameRate * time;
  if (loop) {
    frameTime = std::fmod(frameTime, static_cast<float>(frameCount));
    if (frameTime < 0.f) // std::fmod keeps the sign of the dividend
      frameTime += static_cast<float>(frameCount);
  } else
    frameTime = glm::clamp(frameTime, 0.f, 0.9999f * frameCount);

  uint32_t frameNumber = static_cast<uint32_t>(frameTime);

  for (uint32_t i = 0; i < boneFrameRange.size(); ++i) {
    auto [first, last] = boneFrameRange[i];
    if (first == last) {
      pose.localBoneTransform(i) = Transform::identity;
      continue;
    }

    // Index of the first keyframe with frameNumber > the current frame.
    auto beginIt = boneFrames.begin() + first;
    auto endIt = boneFrames.begin() + last;
    auto succ = std::upper_bound(
        beginIt, endIt, frameNumber,
        [](uint32_t f, const BoneKeyFrame &kf) { return f < kf.frameNumber; });

    if (succ == beginIt) {
      const auto &frame = *beginIt;

      float t = frame.frameNumber == 0 ? 0.f : frameTime / frame.frameNumber;
      glm::vec3 tt{evalCurve(frame.xCurve, t), evalCurve(frame.yCurve, t),
                   evalCurve(frame.zCurve, t)};
      float tr = evalCurve(frame.rCurve, t);

      pose.localBoneTranslation(i) = tt * frame.transform.translation;
      pose.localBoneRotation(i) =
          glm::slerp(glm::identity<glm::quat>(), frame.transform.rotation, tr);
    } else if (succ == endIt) {
      pose.localBoneTransform(i) = (endIt - 1)->transform;
    } else {
      const auto &leftFrame = *(succ - 1);
      const auto &rightFrame = *succ;

      float t = (frameTime - leftFrame.frameNumber) /
                (rightFrame.frameNumber - leftFrame.frameNumber);
      glm::vec3 tt{evalCurve(leftFrame.xCurve, t),
                   evalCurve(leftFrame.yCurve, t),
                   evalCurve(leftFrame.zCurve, t)};
      float tr = evalCurve(leftFrame.rCurve, t);

      pose.localBoneTranslation(i) =
          (1.f - tt) * leftFrame.transform.translation +
          tt * rightFrame.transform.translation;
      pose.localBoneRotation(i) = glm::slerp(leftFrame.transform.rotation,
                                             rightFrame.transform.rotation, tr);
    }
  }

  for (uint32_t i = 0; i < morphFrameRange.size(); ++i) {
    auto [first, last] = morphFrameRange[i];
    if (first == last) {
      pose.morphWeight(i) = 0.f;
      continue;
    }

    auto beginIt = morphFrames.begin() + first;
    auto endIt = morphFrames.begin() + last;
    auto succ = std::upper_bound(
        beginIt, endIt, frameNumber,
        [](uint32_t f, const MorphKeyFrame &kf) { return f < kf.frameNumber; });

    if (succ == beginIt) {
      const auto &frame = *beginIt;
      float t = frame.frameNumber == 0 ? 0.f : frameTime / frame.frameNumber;
      pose.morphWeight(i) = t * frame.weight;
    } else if (succ == endIt) {
      pose.morphWeight(i) = (endIt - 1)->weight;
    } else {
      const auto &leftFrame = *(succ - 1);
      const auto &rightFrame = *succ;
      float t = (frameTime - leftFrame.frameNumber) /
                (rightFrame.frameNumber - leftFrame.frameNumber);
      pose.morphWeight(i) = glm::mix(leftFrame.weight, rightFrame.weight, t);
    }
  }
}

} // namespace glmmd
