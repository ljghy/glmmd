#include <cmath>

#include <glmmd/core/CameraMotion.h>

namespace glmmd
{

CameraMotion::CameraMotion(bool loop_, float frameRate_)
    : loop(loop_)
    , frameRate(frameRate_)
    , frameCount(1)
{
}

void CameraMotion::updateCamera(float time, Camera &camera) const
{
    if (frameCount == 0)
        return;

    float frameTime = frameRate * time;
    if (loop)
        frameTime = std::fmod(frameTime, static_cast<float>(frameCount));
    else
        frameTime = glm::clamp(frameTime, 0.f, 0.9999f * frameCount);

    uint32_t frameNumber = static_cast<int32_t>(frameTime);

    if (frameIndex.empty())
        return;

    auto iter = frameIndex.upper_bound(frameNumber);
    if (iter == frameIndex.begin())
    {
        camera.distance = 0.f;
        camera.target   = glm::vec3(0.f);
        camera.rotation = glm::vec3(0.f);
        camera.fov      = glm::radians(45.f);
        camera.projType = Camera::Perspective;
    }
    else if (iter == frameIndex.end())
    {
        const auto &frame = keyFrames[frameIndex.rbegin()->second];

        camera.distance = frame.distance;
        camera.target   = frame.target;
        camera.rotation = frame.rotation;
        camera.fov      = frame.fov;
        camera.projType =
            frame.perspective ? Camera::Perspective : Camera::Orthographic;
    }
    else
    {
        auto        succ       = iter--;
        const auto &leftFrame  = keyFrames[iter->second];
        const auto &rightFrame = keyFrames[succ->second];

        float t = (frameTime - iter->first) / (succ->first - iter->first);

        float td = evalCurve(leftFrame.distanceCurve, t);
        camera.distance =
            (1.f - td) * leftFrame.distance + td * rightFrame.distance;

        glm::vec3 tt{evalCurve(leftFrame.targetXCurve, t),
                     evalCurve(leftFrame.targetYCurve, t),
                     evalCurve(leftFrame.targetZCurve, t)};
        camera.target = (1.f - tt) * leftFrame.target + tt * rightFrame.target;

        float tr = evalCurve(leftFrame.rotationCurve, t);
        camera.rotation =
            glm::slerp(leftFrame.rotation, rightFrame.rotation, tr);

        float tv   = evalCurve(leftFrame.fovCurve, t);
        camera.fov = (1.f - tv) * leftFrame.fov + tv * rightFrame.fov;

        camera.projType =
            leftFrame.perspective ? Camera::Perspective : Camera::Orthographic;
    }

    camera.update();
}

} // namespace glmmd
