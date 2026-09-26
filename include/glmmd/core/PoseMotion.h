#ifndef GLMMD_CORE_POSE_MOTION_H_
#define GLMMD_CORE_POSE_MOTION_H_

#include <glmmd/core/Motion.h>
#include <glmmd/core/Pose.h>

namespace glmmd {

class PoseMotion : public Motion {
public:
  PoseMotion(const Pose &pose) : m_pose(pose) {}

  PoseMotion(Pose &&pose) : m_pose(std::move(pose)) {}

  float duration() const override { return 0.f; }

  void eval(float time, Pose &pose) const override { pose = m_pose; }

private:
  Pose m_pose;
};

} // namespace glmmd

#endif
