#include "MaterialExtension.h"
#include "JsonParser.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace {
std::string normalizedPath(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  const auto normalized =
      std::filesystem::path(std::u8string(path.begin(), path.end()))
          .lexically_normal()
          .generic_u8string();
  return {normalized.begin(), normalized.end()};
}
} // namespace

MaterialExtensions parseMaterialExtensions(const glmmd::ModelData &data) {
  MaterialExtensions result;
  result.materials.resize(data.materials.size());
  result.texturePaths = data.texturePaths;
  std::unordered_map<std::string, int32_t> textures;
  for (size_t i = 0; i < result.texturePaths.size(); ++i)
    textures.emplace(normalizedPath(result.texturePaths[i]),
                     static_cast<int32_t>(i));

  auto textureIndex = [&](const JsonNode &json, const char *key) -> int32_t {
    const auto value = json.find(key);
    if (value == json.obj().end() || !value->second.isStr() ||
        value->second.str().empty())
      return -1;
    auto path = normalizedPath(value->second.str());
    auto [it, inserted] = textures.emplace(
        path, static_cast<int32_t>(result.texturePaths.size()));
    if (inserted)
      result.texturePaths.push_back(std::move(path));
    return it->second;
  };

  constexpr std::string_view marker = "extension:";
  for (size_t i = 0; i < data.materials.size(); ++i) {
    const auto &material = data.materials[i];
    const auto start = material.memo.find(marker);
    if (start == std::string::npos)
      continue;
    try {
      const auto json = parseJsonString(
          std::string_view(material.memo).substr(start + marker.size()));
      if (!json.isObj()) {
        std::cerr << "Ignoring non-object material extension: " << material.name
                  << '\n';
        continue;
      }
      auto &extension = result.materials[i];
      const auto alpha = json.find("alpha_mode");
      if (alpha != json.obj().end() && alpha->second.isStr()) {
        const auto &mode = alpha->second.str();
        if (mode == "OPAQUE")
          extension.alphaMode = MaterialAlphaMode::Opaque;
        else if (mode == "MASK" || mode == "BINARY")
          extension.alphaMode = MaterialAlphaMode::Mask;
        else if (mode == "BLEND")
          extension.alphaMode = MaterialAlphaMode::Blend;
      }
      extension.normalTextureIndex = textureIndex(json, "normal_texture");
      extension.occlusionTextureIndex = textureIndex(json, "occlusion_texture");
      // transparent_with_z_write and unknown keys are deliberately ignored.
    } catch (const std::runtime_error &error) {
      std::cerr << "Ignoring invalid material extension for " << material.name
                << ": " << error.what() << '\n';
    }
  }
  return result;
}
