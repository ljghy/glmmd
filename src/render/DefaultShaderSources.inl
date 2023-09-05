const char *defaultVertShaderSrc =
    R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec2 aUV;
    uniform mat4 u_model;
    uniform mat4 u_MVP;
    out vec3 normal;
    out vec2 uv;
    void main() {
        normal = normalize(mat3(transpose(inverse(u_model))) * aNormal);
        uv = aUV;
        gl_Position = u_MVP * vec4(aPos, 1.0);
    }
    )";

const char *defaultFragShaderSrc =
    R"(
    #version 330 core
    in vec3 normal;
    in vec2 uv;
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
    uniform vec3 u_lightDir;
    uniform vec3 u_lightColor;
    uniform vec3 u_ambientColor;
    out vec4 FragColor;
    vec4 applyMul(vec4 color, vec4 factor) {
        vec3 k = mix(vec3(1.0, 1.0, 1.0), factor.rgb, factor.a);
        return vec4(color.rgb * k, color.a);
    }
    vec4 applyAdd(vec4 color, vec4 factor) {
        return vec4(color.rgb + factor.rgb * factor.a, color.a);
    }
    void main() {
        vec3 norm = normalize(normal);
        vec3 viewDir = -normalize(u_viewDir);
        vec3 lightDir = -normalize(u_lightDir);
        vec3 phong = u_mat.diffuse.rgb * u_lightColor;
        vec3 halfVec = normalize(viewDir + lightDir);
        if (u_mat.specularPower > 0.0) {
            float spec = pow(max(dot(norm, halfVec), 0.0),
                             u_mat.specularPower);
            phong += u_mat.specular * spec;
        }
        phong += u_mat.ambient * u_ambientColor;
        phong = clamp(phong, 0.0, 1.0);
        vec4 color = vec4(phong, u_mat.diffuse.a);
        if (u_mat.hasTexture == 1) {
            color *= texture(u_mat.texture, uv);
            if (color.a == 0.0) discard;
            color = applyMul(color, u_mat.textureMul);
            color = applyAdd(color, u_mat.textureAdd);
        }
        if (u_mat.sphereTextureMode == 1 || u_mat.sphereTextureMode == 2) {
            vec3 t = normalize(cross(vec3(0.0, 1.0, 0.0), viewDir));
            vec3 b = cross(viewDir, t);
            vec2 spUV = vec2(dot(norm, t), -dot(norm, b));
            spUV = 0.5 + 0.5 * spUV;
            vec4 spColor = texture(u_mat.sphereTexture, spUV);
            spColor = applyMul(spColor, u_mat.sphereTextureMul);
            spColor = applyAdd(spColor, u_mat.sphereTextureAdd);
            if (u_mat.sphereTextureMode == 1)
                color *= spColor;
            else
                color = vec4(spColor.rgb + color.rgb, color.a);
        }
        float visibility = 0.5 * dot(norm, lightDir) + 0.5;
        if (u_mat.hasToonTexture == 1) {
            vec2 toonUV = vec2(0.5, clamp(1.0 - visibility, 0.0, 1.0));
            vec4 toonColor = texture(u_mat.toonTexture, toonUV);
            toonColor = applyMul(toonColor, u_mat.toonTextureMul);
            toonColor = applyAdd(toonColor, u_mat.toonTextureAdd);
            color *= toonColor;
        }
        FragColor = color;
    }
    )";

const char *defaultEdgeVertShaderSrc =
    R"(
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

const char *defaultEdgeFragShaderSrc =
    R"(
    #version 330 core
    uniform vec4 u_edgeColor;
    out vec4 FragColor;
    void main() {
        FragColor = u_edgeColor;
    }
    )";
