#ifndef GLMMD_CORE_FIXED_POSE_MOTION_H_
#define GLMMD_CORE_FIXED_POSE_MOTION_H_

#include <glmmd/core/ModelPose.h>
#include <glmmd/core/Motion.h>

namespace glmmd
{

class FixedPoseMotion : public Motion
{
public:
    FixedPoseMotion(const ModelPose &pose)
        : m_pose(pose)
    {
    }

    FixedPoseMotion(ModelPose &&pose)
        : m_pose(std::move(pose))
    {
    }

    float duration() const override { return 0.f; }

    void getLocalPose(float time, ModelPose &pose) const override
    {
        pose = m_pose;
    }

private:
    ModelPose m_pose;
};

} // namespace glmmd

#endif
