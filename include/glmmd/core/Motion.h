#ifndef GLMMD_CORE_MOTION_H_
#define GLMMD_CORE_MOTION_H_

#include <glmmd/core/Pose.h>

namespace glmmd {

// Abstract animation source. Given a time in seconds, writes local bone
// transforms and morph weights into a Pose. Implementations include MotionClip
// (VMD keyframes) and PoseMotion (a static pose).
class Motion {
public:
  virtual float duration() const = 0;
  virtual void eval(float time, Pose &pose) const = 0;

  virtual ~Motion() = default;
};

} // namespace glmmd

#endif