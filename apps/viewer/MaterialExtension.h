#ifndef GLMMD_VIEWER_MATERIAL_EXTENSION_H_
#define GLMMD_VIEWER_MATERIAL_EXTENSION_H_

#include <glmmd/core/ModelData.h>

enum class MaterialAlphaMode { Blend, Opaque, Mask };

struct MaterialExtension {
  MaterialAlphaMode alphaMode = MaterialAlphaMode::Blend;
  int32_t normalTextureIndex = -1;
  int32_t occlusionTextureIndex = -1;
};

struct MaterialExtensions {
  std::vector<MaterialExtension> materials;
  // Original PMX paths followed by interned extension paths. Indices are local
  // to the renderer: parsing never changes the shared ModelData asset.
  std::vector<std::string> texturePaths;
};

MaterialExtensions parseMaterialExtensions(const glmmd::ModelData &data);

#endif
