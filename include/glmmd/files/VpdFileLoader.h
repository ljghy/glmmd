#ifndef GLMMD_FILES_VPD_FILE_LOADER_H_
#define GLMMD_FILES_VPD_FILE_LOADER_H_

#include <glmmd/files/VpdData.h>

#include <filesystem>

namespace glmmd {

// Loads a VPD pose file. Throws std::runtime_error on I/O errors or malformed
// data.
VpdData loadVpdFile(const std::filesystem::path &path);

} // namespace glmmd

#endif
