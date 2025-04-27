#ifndef GLMMD_CORE_CAMERA_MOTION_H_
#define GLMMD_CORE_CAMERA_MOTION_H_

#include <map>
#include <vector>

#include <glmmd/core/Camera.h>
#include <glmmd/core/InterpolationCurve.h>

namespace glmmd
{

struct CameraMotion
{
    struct CameraKeyFrame
    {
        float     distance;
        glm::vec3 target;
        glm::quat rotation;

        float fov; // rad
        bool  perspective;

        InterpolationCurvePoints distanceCurve;
        InterpolationCurvePoints targetXCurve;
        InterpolationCurvePoints targetYCurve;
        InterpolationCurvePoints targetZCurve;
        InterpolationCurvePoints rotationCurve;
        InterpolationCurvePoints fovCurve;
    };

    CameraMotion(bool loop = false, float frameRate = 30.f);

    float duration() const { return frameCount / frameRate; }

    void updateCamera(float time, Camera &camera) const;

    bool  loop;
    float frameRate;

    uint32_t frameCount;

    std::map<uint32_t, uint32_t> frameIndex;
    std::vector<CameraKeyFrame>  keyFrames;
};

} // namespace glmmd

#endif
