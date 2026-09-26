#ifndef GLMMD_FILES_PMX_FILE_DUMPER_H_
#define GLMMD_FILES_PMX_FILE_DUMPER_H_

#include <glmmd/core/ModelData.h>

#include <filesystem>

namespace glmmd {

void dumpPmxFile(const std::filesystem::path &path, const ModelData &data);

} // namespace glmmd

#endif
