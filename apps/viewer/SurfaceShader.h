#ifndef GLMMD_VIEWER_SURFACE_SHADER_H_
#define GLMMD_VIEWER_SURFACE_SHADER_H_

#include <string>

#include <glmmd/core/Camera.h>
#include <opengl_framework/Shader.h>

enum class SurfacePass { Opaque, Transparent };

// Fragment sources call writeSurface with straight, linear RGBA and window
// depth. Inject after #version so custom material shaders share the same output
// and alpha partition as meshes, outlines, and the grid.
inline std::string surfaceFragmentShader(std::string source) {
  const auto versionEnd = source.find('\n', source.find("#version"));
  source.insert(versionEnd + 1, R"(
uniform bool u_transparentPass;
uniform bool u_orthographic;
uniform vec2 u_depthRange;
layout(location = 0) out vec4 surfaceColor;
layout(location = 1) out float surfaceRevealage;

vec3 displayToLinear(vec3 color) {
    return mix(color / 12.92, pow(max((color + 0.055) / 1.055, vec3(0.0)), vec3(2.4)),
               step(vec3(0.04045), color));
}

void writeSurface(vec4 color, float windowDepth) {
    float alpha = clamp(color.a, 0.0, 1.0);
    if (alpha == 0.0 || (u_transparentPass ? alpha == 1.0 : alpha < 1.0))
        discard;
    if (!u_transparentPass) {
        surfaceColor = vec4(color.rgb, 1.0);
        surfaceRevealage = 0.0;
        return;
    }
    float near = u_depthRange.x;
    float far = u_depthRange.y;
    float depth = u_orthographic ? mix(near, far, windowDepth)
        : near * far / (far - windowDepth * (far - near));
    // Favor opaque and nearby layers. 50 is a view-space MMD distance scale;
    // the bounded weight leaves FP16 headroom for dense overlapping surfaces.
    float weight = clamp(pow(alpha + 0.01, 3.0) * 32.0 /
                         (1.0 + pow(depth / 50.0, 2.0)), 0.01, 32.0);
    surfaceColor = vec4(color.rgb * alpha, alpha) * weight;
    surfaceRevealage = alpha;
}
)");
  return source;
}

inline void setSurfacePass(const ogl::Shader &shader, SurfacePass pass,
                           const glmmd::Camera &camera) {
  shader.setUniform1i("u_transparentPass", pass == SurfacePass::Transparent);
  shader.setUniform1i("u_orthographic",
                      camera.projType == glmmd::Camera::Orthographic);
  const glm::vec2 depthRange(camera.zNear, camera.zFar);
  shader.setUniform2fv("u_depthRange", &depthRange[0]);
}

#endif
