#ifndef OPENGL_SHADER_H_
#define OPENGL_SHADER_H_

#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader();
    ~Shader();

    void create(const char *vertSrc, const char *fragSrc,
                const char *geometrySrc = nullptr);

    Shader(const Shader &)            = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&other);

    unsigned int getId() const;

    void use() const;
    void destroy();

    void setUniform1i(const std::string &name, int n);
    void setUniform2fv(const std::string &name, const float *v);
    void setUniform3fv(const std::string &name, const float *v);
    void setUniform1f(const std::string &name, float f);
    void setUniform4fv(const std::string &name, const float *v);
    void setUniformMatrix4fv(const std::string &name, const float *ptr,
                             bool transpose = false, unsigned int count = 1u);

private:
    int getUniformLocation(const std::string &name);

private:
    unsigned int m_id;

    std::unordered_map<std::string, int> m_uniformLocationCache;
};

#endif