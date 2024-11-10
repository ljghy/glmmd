#ifndef GLMMD_FILES_VPD_DATA_H_
#define GLMMD_FILES_VPD_DATA_H_

#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glmmd/core/ModelPose.h>

namespace glmmd
{

struct VpdData
{
    ModelPose
    toModelPose(const std::shared_ptr<const ModelData> &modelData) const;

    std::string modelName; // Shift-JIS

    struct Bone
    {
        std::string name; // Shift-JIS

        glm::vec3 translation;
        glm::quat rotation;
    };
    std::vector<Bone> bones;
};

} // namespace glmmd

#endif
