#ifndef GLMMD_FILES_VMD_DATA_H_
#define GLMMD_FILES_VMD_DATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <glmmd/core/CameraMotion.h>
#include <glmmd/core/FixedMotionClip.h>
#include <glmmd/core/ModelData.h>

namespace glmmd
{

struct VmdData
{
    FixedMotionClip toFixedMotionClip(const ModelData &modelData,
                                      bool             loop      = false,
                                      float            frameRate = 30.f) const;

    CameraMotion toCameraMotion(bool  loop      = false,
                                float frameRate = 30.f) const;

    int         version;
    std::string modelName; // Shift-JIS

    struct BoneKeyFrame
    {
        std::string boneName; // Shift-JIS

        uint32_t frameNumber;

        glm::vec3 translation;
        glm::quat rotation;

        uint8_t interpolation[64];
    };
    std::vector<BoneKeyFrame> boneFrames;

    struct MorphKeyFrame
    {
        std::string morphName; // Shift-JIS

        uint32_t frameNumber;

        float ratio;
    };
    std::vector<MorphKeyFrame> morphFrames;

    struct CameraKeyFrame
    {
        uint32_t frameNumber;

        float     distance;
        glm::vec3 target;
        glm::vec3 rotation;

        uint8_t interpolation[24]; // x, y, z, rotation, distance, fov

        uint32_t fov;         // deg
        uint8_t  perspective; // 0: Perspective, 1: Orthographic
    };
    std::vector<CameraKeyFrame> cameraFrames;

    bool isCameraMotion() const { return !cameraFrames.empty(); }
};

} // namespace glmmd

#endif
