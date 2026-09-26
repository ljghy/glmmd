#ifndef GLMMD_CORE_CAMERA_MOTION_H_
#define GLMMD_CORE_CAMERA_MOTION_H_

#include <glmmd/core/Camera.h>
#include <glmmd/core/InterpolationCurve.h>

#include <cstdint>
#include <vector>

namespace glmmd {

struct CameraMotion {
  struct CameraKeyFrame {
    uint32_t frameNumber;

    float distance;
    glm::vec3 target;
    glm::quat rotation;

    float fov; // rad
    bool perspective;

    InterpolationCurveNodes distanceCurve;
    InterpolationCurveNodes targetXCurve;
    InterpolationCurveNodes targetYCurve;
    InterpolationCurveNodes targetZCurve;
    InterpolationCurveNodes rotationCurve;
    InterpolationCurveNodes fovCurve;
  };

  CameraMotion(bool loop = false, float frameRate = 30.f);

  float duration() const { return frameCount / frameRate; }

  void updateCamera(float time, Camera &camera) const;

  bool loop;
  float frameRate;

  uint32_t frameCount;

  // Keyframes sorted by frame number; updateCamera binary-searches this.
  std::vector<CameraKeyFrame> keyFrames;
};

} // namespace glmmd

#endif
