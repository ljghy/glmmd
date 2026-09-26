#include <glmmd/core/CameraMotion.h>

#include <algorithm>
#include <cmath>

namespace glmmd {

CameraMotion::CameraMotion(bool loop_, float frameRate_)
    : loop(loop_), frameRate(frameRate_), frameCount(1) {}

void CameraMotion::updateCamera(float time, Camera &camera) const {
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

  if (keyFrames.empty())
    return;

  auto succ = std::upper_bound(
      keyFrames.begin(), keyFrames.end(), frameNumber,
      [](uint32_t f, const CameraKeyFrame &kf) { return f < kf.frameNumber; });

  if (succ == keyFrames.begin()) {
    camera.distance = 0.f;
    camera.target = glm::vec3(0.f);
    camera.rotation = glm::vec3(0.f);
    camera.fov = glm::radians(45.f);
    camera.projType = Camera::Perspective;
  } else if (succ == keyFrames.end()) {
    const auto &frame = keyFrames.back();

    camera.distance = frame.distance;
    camera.target = frame.target;
    camera.rotation = frame.rotation;
    camera.fov = frame.fov;
    camera.projType =
        frame.perspective ? Camera::Perspective : Camera::Orthographic;
  } else {
    const auto &leftFrame = *(succ - 1);
    const auto &rightFrame = *succ;

    float t = (frameTime - leftFrame.frameNumber) /
              (rightFrame.frameNumber - leftFrame.frameNumber);

    float td = evalCurve(leftFrame.distanceCurve, t);
    camera.distance =
        (1.f - td) * leftFrame.distance + td * rightFrame.distance;

    glm::vec3 tt{evalCurve(leftFrame.targetXCurve, t),
                 evalCurve(leftFrame.targetYCurve, t),
                 evalCurve(leftFrame.targetZCurve, t)};
    camera.target = (1.f - tt) * leftFrame.target + tt * rightFrame.target;

    float tr = evalCurve(leftFrame.rotationCurve, t);
    camera.rotation = glm::slerp(leftFrame.rotation, rightFrame.rotation, tr);

    float tv = evalCurve(leftFrame.fovCurve, t);
    camera.fov = (1.f - tv) * leftFrame.fov + tv * rightFrame.fov;

    camera.projType =
        leftFrame.perspective ? Camera::Perspective : Camera::Orthographic;
  }

  camera.update();
}

} // namespace glmmd
