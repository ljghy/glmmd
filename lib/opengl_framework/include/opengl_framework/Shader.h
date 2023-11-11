#ifndef OPENGL_SHADER_H_
#define OPENGL_SHADER_H_

#include <string>
#include <unordered_map>

#include <opengl_framework/APIConfig.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API Shader
{
public:
    Shader();
    ~Shader();

    void create(const char *vertSrc, const char *fragSrc,
                const char *geomSrc = nullptr);

    void createFromFile(const char *vertPath, const char *fragPath,
                        const char *geomPath = nullptr);

    Shader(const Shader &)            = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&other);

    unsigned int getId() const;

    void use() const;
    void destroy();

    void setUniform1i(const std::string &name, int n) const;
    void setUniform2fv(const std::string &name, const float *v) const;
    void setUniform3fv(const std::string &name, const float *v) const;
    void setUniform1f(const std::string &name, float f) const;
    void setUniform4fv(const std::string &name, const float *v) const;
    void setUniformMatrix4fv(const std::string &name, const float *ptr,
                             bool         transpose = false,
                             unsigned int count     = 1u) const;

private:
    int getUniformLocation(const std::string &name) const;

private:
    unsigned int m_id;

    mutable std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace ogl

#endif