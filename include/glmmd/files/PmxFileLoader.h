#ifndef GLMMD_FILES_PMX_FILE_LOADER_H_
#define GLMMD_FILES_PMX_FILE_LOADER_H_

#include <glmmd/core/ModelData.h>

#include <filesystem>

namespace glmmd {

// Loads and validates a PMX model file. Throws std::runtime_error on I/O
// errors, malformed data, or out-of-range indices. The caller owns the result
// and may wrap it in a shared_ptr for use with Model/Pose/ModelRenderData.
ModelData loadPmxFile(const std::filesystem::path &path);

} // namespace glmmd

#endif