#ifndef GLMMD_FILE_VMD_DATA_H_
#define GLMMD_FILE_VMD_DATA_H_

#include <string>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/FixedMotionClip.h>

namespace glmmd
{

struct VmdData
{
    FixedMotionClip toFixedMotionClip(const ModelData &modelData,
                                      bool             loop      = false,
                                      float            frameRate = 30.f) const;

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
        glm::vec3 targetPosition;
        glm::vec3 rotation;

        uint8_t interpolation[24];

        uint32_t fov;
        uint8_t  perspective;
    };
    std::vector<CameraKeyFrame> cameraFrames;
};

} // namespace glmmd

#endif
