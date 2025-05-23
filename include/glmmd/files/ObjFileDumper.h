#ifndef GLMMD_FILES_PMX_FILE_DUMPER_H_
#define GLMMD_FILES_PMX_FILE_DUMPER_H_

#include <filesystem>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelRenderData.h>

namespace glmmd
{

void dumpObjFile(const std::filesystem::path &path, const ModelData &modelData,
                 const ModelRenderData *renderData = nullptr);

}

#endif
