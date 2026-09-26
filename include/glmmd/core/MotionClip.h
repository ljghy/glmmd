#ifndef GLMMD_CORE_MOTION_CLIP_H_
#define GLMMD_CORE_MOTION_CLIP_H_

#include <glmmd/core/InterpolationCurve.h>
#include <glmmd/core/Motion.h>
#include <glmmd/core/Transform.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace glmmd {

// A keyframed motion (typically converted from a VMD file via
// VmdData::toMotionClip) with per-bone and per-morph keyframes and Bezier
// interpolation curves. Bones/morphs are pre-resolved to model indices.
//
// Keyframes are stored contiguously and grouped by bone/morph index, sorted by
// frame number within each group. `boneFrameRange[i]` / `morphFrameRange[i]`
// give the half-open [first, last) span into `boneFrames` / `morphFrames` for
// bone/morph `i`; eval() binary-searches within that span. This avoids the
// per-node allocation and pointer chasing of a per-bone std::map.
struct MotionClip : public Motion {
  struct BoneKeyFrame {
    uint32_t frameNumber;

    Transform transform;

    InterpolationCurveNodes xCurve;
    InterpolationCurveNodes yCurve;
    InterpolationCurveNodes zCurve;
    InterpolationCurveNodes rCurve;
  };

  struct MorphKeyFrame {
    uint32_t frameNumber;

    float weight;
  };

  MotionClip(bool loop = false, float frameRate = 30.f);

  float duration() const override { return frameCount / frameRate; }

  void eval(float time, Pose &pose) const override;

  bool loop;
  float frameRate;

  uint32_t frameCount;

  std::vector<std::pair<uint32_t, uint32_t>> boneFrameRange;
  std::vector<std::pair<uint32_t, uint32_t>> morphFrameRange;

  std::vector<BoneKeyFrame> boneFrames;
  std::vector<MorphKeyFrame> morphFrames;
};

} // namespace glmmd

#endif
