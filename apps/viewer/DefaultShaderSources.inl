const char *defaultVertShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
///ADDITIONAL_UV_LAYOUT///
uniform mat4 u_model;
uniform mat4 u_MVP;
uniform mat4 u_lightVP;
out vec3 normal;
out vec2 uv;
///ADDITIONAL_UV_OUT///
out vec3 fragPos;
out vec4 fragPosLightSpace;
void main() {
    normal = normalize(mat3(transpose(inverse(u_model))) * aNormal);
    uv = aUV;
    ///ADDITIONAL_UV_V2F///
    gl_Position = u_MVP * vec4(aPos, 1.0);
    fragPos = vec3(u_model * vec4(aPos, 1.0));
    fragPosLightSpace = u_lightVP * vec4(fragPos, 1.0);
}
)";

const char *defaultFragShaderSrc = R"(
#version 330 core
in vec3 normal;
in vec2 uv;
///ADDITIONAL_UV_IN///
in vec3 fragPos;
in vec4 fragPosLightSpace;
struct Material {
    vec4 diffuse;
    vec3 specular;
    float specularPower;
    vec3 ambient;
    vec4 edgeColor;
    float edgeSize;
    int hasTexture;
    vec4 textureAdd;
    vec4 textureMul;
    sampler2D texture;
    int sphereTextureMode;
    vec4 sphereTextureAdd;
    vec4 sphereTextureMul;
    sampler2D sphereTexture;
    int hasToonTexture;
    vec4 toonTextureAdd;
    vec4 toonTextureMul;
    sampler2D toonTexture;
};
uniform Material u_mat;
uniform vec3 u_viewDir;
uniform mat4 u_view;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform vec3 u_ambientColor;
uniform int u_receiveShadow;
uniform int u_hasShadowMap;
uniform sampler2DShadow u_shadowMap;
uniform mat4 u_lightVP;
uniform vec3 u_shadowBias;
uniform float u_shadowTexelWorld;
uniform float u_shadowDepthSpan;
uniform int u_shadowRadius;
uniform vec2 u_shadowFade;
uniform vec4 u_shadowViewDepth;
uniform bool u_hasNormalTexture;
uniform sampler2D u_normalTexture;
uniform bool u_hasOcclusionTexture;
uniform sampler2D u_occlusionTexture;

vec4 applyMul(vec4 color, vec4 factor) {
    // PMX factor alpha controls texture influence, not surface opacity.
    return vec4(mix(vec3(1.0), color.rgb * factor.rgb, factor.a), color.a);
}
vec4 applyAdd(vec4 color, vec4 factor) {
    return vec4(clamp(color.rgb + (color.rgb - vec3(1.0)) * factor.a,
                      0.0, 1.0) + factor.rgb, color.a);
}
vec3 mappedNormal(vec3 geometricNormal) {
    if (!u_hasNormalTexture) return geometricNormal;
    // Construct a tangent frame from deformed positions and UVs. This keeps
    // normal maps aligned with skinning/UV morphs without a tangent vertex stream.
    vec3 dx = dFdx(fragPos), dy = dFdy(fragPos);
    vec2 tx = dFdx(uv), ty = dFdy(uv);
    float determinant = tx.x * ty.y - tx.y * ty.x;
    if (abs(determinant) < 1e-10) return geometricNormal;
    vec3 tangent = (dx * ty.y - dy * tx.y) / determinant;
    vec3 bitangent = (dy * tx.x - dx * ty.x) / determinant;
    tangent -= geometricNormal * dot(geometricNormal, tangent);
    if (dot(tangent, tangent) < 1e-10) return geometricNormal;
    tangent = normalize(tangent);
    float handedness = dot(cross(geometricNormal, tangent), bitangent) < 0.0 ? -1.0 : 1.0;
    bitangent = cross(geometricNormal, tangent) * handedness;
    vec3 mapped = texture(u_normalTexture, uv).xyz * 2.0 - 1.0;
    vec3 normal = mat3(tangent, bitangent, geometricNormal) * mapped;
    return dot(normal, normal) > 1e-10 ? normalize(normal) : geometricNormal;
}
float shadowVisibility(vec3 normal, vec3 lightDir) {
    // Bias faces toward the light even for double-sided/back-facing receivers.
    vec3 biasNormal = dot(normal, lightDir) < 0.0 ? -normal : normal;
    float cosine = clamp(dot(biasNormal, lightDir), 0.0, 1.0);
    float sine = sqrt(max(0.0, 1.0 - cosine * cosine));
    float slope = min(sine / max(cosine, 0.1), 4.0);
    vec3 normalOffset = biasNormal * (u_shadowBias.z * sine * u_shadowTexelWorld);
    vec4 lightPos = fragPosLightSpace + u_lightVP * vec4(normalOffset, 0.0);
    vec3 coord = lightPos.xyz / lightPos.w * 0.5 + 0.5;
    if (any(lessThan(coord, vec3(0.0))) || any(greaterThan(coord, vec3(1.0))))
        return 1.0;
    float bias = (u_shadowBias.x + u_shadowBias.y * slope) *
                 u_shadowTexelWorld / u_shadowDepthSpan;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));
    float visibility = 0.0;
    float totalWeight = 0.0;
    for (int y = -u_shadowRadius; y <= u_shadowRadius; ++y) {
        for (int x = -u_shadowRadius; x <= u_shadowRadius; ++x) {
            float weight = float((u_shadowRadius + 1 - abs(x)) *
                                 (u_shadowRadius + 1 - abs(y)));
            visibility += weight * texture(u_shadowMap,
                vec3(coord.xy + vec2(x, y) * texelSize, coord.z - bias));
            totalWeight += weight;
        }
    }
    visibility /= totalWeight;
    float viewDepth = dot(u_shadowViewDepth, vec4(fragPos, 1.0));
    float fade = smoothstep(u_shadowFade.x, u_shadowFade.y, viewDepth);
    return mix(visibility, 1.0, fade);
}
void main() {
    vec3 geometricNormal = normalize(normal);
    vec3 norm = mappedNormal(geometricNormal);
    vec3 viewDir = -normalize(u_viewDir);
    vec3 lightDir = -normalize(u_lightDir);
    float shadowLight = 1.0;
    if (u_hasShadowMap * u_receiveShadow > 0)
        shadowLight = shadowVisibility(geometricNormal, lightDir);
    vec3 phong = u_mat.diffuse.rgb * u_lightColor;
    vec3 halfVec = normalize(viewDir + lightDir);
    if (u_mat.specularPower > 0.0) {
        float spec = pow(max(dot(norm, halfVec), 0.0),
                            u_mat.specularPower);
        phong += u_mat.specular * spec;
    }
    // Toon materials retain the PMX ramp response below. Without a toon
    // texture, shadow direct diffuse/specular while preserving ambient light.
    if (u_mat.hasToonTexture == 0)
        phong *= shadowLight;
    float occlusion = u_hasOcclusionTexture ? texture(u_occlusionTexture, uv).r : 1.0;
    phong += u_mat.ambient * u_ambientColor * occlusion;
    phong = clamp(phong, 0.0, 1.0);
    vec4 color = vec4(phong, u_mat.diffuse.a);
    if (u_mat.hasTexture == 1) {
        vec4 baseColor = texture(u_mat.texture, uv);
        baseColor = applyMul(baseColor, u_mat.textureMul);
        baseColor = applyAdd(baseColor, u_mat.textureAdd);
        color *= baseColor;
        if (color.a <= 0.0 && u_alphaMode != 1) discard;
    }
    if (u_mat.sphereTextureMode == 1 || u_mat.sphereTextureMode == 2) {
        // Use the actual camera basis, including roll and vertical views.
        vec3 viewNormal = normalize(mat3(u_view) * norm);
        vec2 spUV = viewNormal.xy * vec2(0.5, -0.5) + 0.5;
        vec4 spColor = texture(u_mat.sphereTexture, spUV);
        spColor = applyMul(spColor, u_mat.sphereTextureMul);
        spColor = applyAdd(spColor, u_mat.sphereTextureAdd);
        if (u_mat.sphereTextureMode == 1)
            color.rgb *= spColor.rgb; // Sphere maps do not change opacity.
        else
            color = vec4(spColor.rgb + color.rgb, color.a);
    }
    else if (u_mat.sphereTextureMode == 3) {
    #ifdef USE_ADDITIONAL_UV
        vec2 spUV = additionalUV1.xy;
    #else
        vec2 spUV = uv;
    #endif
        vec4 spColor = texture(u_mat.sphereTexture, spUV);
        spColor = applyMul(spColor, u_mat.sphereTextureMul);
        spColor = applyAdd(spColor, u_mat.sphereTextureAdd);
        color *= spColor;
    }

    float visibility = 0.5 * dot(norm, lightDir) + 0.5;
    visibility = min(visibility, shadowLight);
    if (u_mat.hasToonTexture == 1) {
        vec2 toonUV = vec2(0.5, clamp(1.0 - visibility, 0.0, 1.0));
        vec4 toonColor = texture(u_mat.toonTexture, toonUV);
        toonColor = applyMul(toonColor, u_mat.toonTextureMul);
        toonColor = applyAdd(toonColor, u_mat.toonTextureAdd);
        color *= toonColor;
    }
    color.a = materialAlpha(color.a);
    writeSurface(color, gl_FragCoord.z);
}
)";

const char *defaultEdgeVertShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 u_MV;
uniform mat4 u_MVP;
uniform vec2 u_viewportSize;
uniform float u_edgeSize;
void main() {
    vec3 norm = normalize(transpose(inverse(mat3(u_MV))) * aNormal);
    vec4 pos = u_MVP * vec4(aPos, 1.0);
    vec2 screenNorm = normalize(norm.xy);
    pos.xy += screenNorm * 2 / u_viewportSize * u_edgeSize * 2 * pos.w;
    gl_Position = pos;
}
)";

const char *defaultEdgeFragShaderSrc = R"(
#version 330 core
uniform vec4 u_edgeColor;

void main() {
    writeSurface(vec4(displayToLinear(u_edgeColor.rgb), u_edgeColor.a), gl_FragCoord.z);
}
)";

const char *defaultShadowMapVertShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
uniform mat4 u_lightVP;
uniform mat4 u_model;
out vec2 uv;
void main() {
    uv = aUV;
    gl_Position = u_lightVP * u_model * vec4(aPos, 1.0);
}
)";

const char *defaultShadowMapFragShaderSrc = R"(
#version 330 core
in vec2 uv;
uniform sampler2D u_texture;
uniform bool u_hasTexture;
uniform float u_diffuseAlpha;
uniform float u_alphaCutoff;
void main() {
    float alpha = u_diffuseAlpha;
    if (u_hasTexture)
        alpha *= texture(u_texture, uv).a;
    alpha = materialAlpha(alpha);
    if (alpha <= 0.0 || alpha < u_alphaCutoff)
        discard;
}
)";

const char *defaultGroundShadowVertShaderSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
uniform mat4 u_MVP;
out float worldY;
out vec2 uv;
void main() {
    worldY = aPos.y;
    uv = aUV;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)";

const char *defaultGroundShadowFragShaderSrc = R"(
#version 410 core
in float worldY;
in vec2 uv;
uniform sampler2D u_texture;
uniform bool u_hasTexture;
uniform float u_diffuseAlpha;

// Fixed screen-space coverage shared by all projected triangles. Matching
// thresholds keep overlapping silhouettes from darkening each other.
const int bayer[16] = int[](
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
);
out vec4 FragColor;
void main() {
    if (worldY < 0.0)
        discard;
    float alpha = u_diffuseAlpha;
    if (u_hasTexture)
        alpha *= texture(u_texture, uv).a;
    alpha = materialAlpha(alpha);
    ivec2 pixel = ivec2(gl_FragCoord.xy) & ivec2(3);
    float threshold = (float(bayer[pixel.y * 4 + pixel.x]) + 0.5) / 16.0;
    // Spread coverage across MSAA samples as well as pixels. No frame-based
    // noise: static geometry and opacity produce identical coverage each frame.
    threshold = fract(threshold + float(gl_SampleID) / float(gl_NumSamples));
    if (alpha <= threshold)
        discard;
    // Surviving samples are opaque depth-writing coverage, not alpha blending.
    // Display-space 0.1 converted to the scene's linear working space.
    FragColor = vec4(0.010022826, 0.010022826, 0.010022826, 1.0);
}
)";
