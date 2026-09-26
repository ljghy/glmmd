#ifndef GLMMD_FILES_OBJ_FILE_DUMPER_H_
#define GLMMD_FILES_OBJ_FILE_DUMPER_H_

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelRenderData.h>

#include <filesystem>

namespace glmmd {

void dumpObjFile(const std::filesystem::path &path, const ModelData &modelData,
                 const ModelRenderData *renderData = nullptr);

}

#endif
