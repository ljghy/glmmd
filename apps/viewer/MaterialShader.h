#ifndef GLMMD_VIEWER_MATERIAL_SHADER_H_
#define GLMMD_VIEWER_MATERIAL_SHADER_H_

#include <string>

// Shared by mesh and both shadow shaders. Values follow MaterialAlphaMode.
inline std::string materialFragmentShader(std::string source) {
  const auto versionEnd = source.find('\n', source.find("#version"));
  source.insert(versionEnd + 1, R"(
uniform int u_alphaMode;
float materialAlpha(float alpha) {
    if (u_alphaMode == 1) return 1.0; // OPAQUE ignores all alpha inputs.
    alpha = clamp(alpha, 0.0, 1.0);
    if (u_alphaMode == 2) return step(0.5, alpha); // MASK / BINARY
    return alpha; // BLEND retains fractional coverage.
}
)");
  return source;
}

#endif
