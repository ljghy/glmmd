#ifndef GLMMD_FILES_VMD_FILE_LOADER_H_
#define GLMMD_FILES_VMD_FILE_LOADER_H_

#include <glmmd/files/VmdData.h>

#include <filesystem>

namespace glmmd {

// Loads a VMD motion/camera file. Throws std::runtime_error on I/O errors or
// malformed data.
VmdData loadVmdFile(const std::filesystem::path &path);

} // namespace glmmd

#endif
