#ifndef GLMMD_CORE_MOTION_H_
#define GLMMD_CORE_MOTION_H_

#include <glmmd/core/ModelPose.h>

namespace glmmd
{

class Motion
{
public:
    virtual float duration() const                                = 0;
    virtual void  getLocalPose(float time, ModelPose &pose) const = 0;

    virtual ~Motion() = default;
};

} // namespace glmmd

#endif