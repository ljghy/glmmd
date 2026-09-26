#ifndef GLMMD_FILES_VPD_DATA_H_
#define GLMMD_FILES_VPD_DATA_H_

#include <glmmd/core/Pose.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

namespace glmmd {

struct VpdData {
  // Builds a Pose for the given model. The returned Pose holds a non-owning
  // pointer to `modelData`, which must outlive the Pose.
  Pose toPose(const ModelData &modelData) const;

  std::string modelName; // Shift-JIS

  struct Bone {
    std::string name; // Shift-JIS

    glm::vec3 translation;
    glm::quat rotation;
  };
  std::vector<Bone> bones;
};

} // namespace glmmd

#endif
