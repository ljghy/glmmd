#ifndef GLMMD_CORE_MOTION_CLIP_H_
#define GLMMD_CORE_MOTION_CLIP_H_

#include <glmmd/core/ModelPose.h>

namespace glmmd
{

class MotionClip
{
public:
    virtual float duration() const                                = 0;
    virtual void  getPoseLocal(float time, ModelPose &pose) const = 0;

    virtual ~MotionClip() = default;

private:
};

} // namespace glmmd

#endif