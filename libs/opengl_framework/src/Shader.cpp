#include <glad/glad.h>

#include <string>
#include <stdexcept>
#include <fstream>

#include <opengl_framework/Shader.h>

namespace ogl
{

Shader::Shader()
    : m_id(0)
{
}

Shader::Shader(Shader &&other)
    : m_id(other.m_id)
{
    other.m_id = 0;
}

void Shader::create(const char *vertSrc, const char *fragSrc,
                    const char *geomSrc)
{
    destroy();

    unsigned int vert, frag, geometry{};
    int          success;
    char         infoLog[512];

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, NULL);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        throw std::runtime_error(std::string("Vertex shader compile error: ") +
                                 infoLog);
    };

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        throw std::runtime_error(
            std::string("Fragment shader compile error: ") + infoLog);
    };

    if (geomSrc)
    {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geomSrc, NULL);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geometry, 512, NULL, infoLog);
            throw std::runtime_error(
                std::string("Geometry shader compile error: ") + infoLog);
        };
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    if (geomSrc)
        glAttachShader(m_id, geometry);
    glLinkProgram(m_id);

    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_id, 512, NULL, infoLog);
        throw std::runtime_error(std::string("Shader link error: ") + infoLog);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    if (geomSrc)
        glDeleteShader(geometry);
}

void Shader::createFromFile(const char *vertPath, const char *fragPath,
                            const char *geomPath)
{
    std::ifstream vertFin(vertPath);
    if (!vertFin.is_open())
        throw std::runtime_error(
            std::string("Failed to load vertex shader from ") + vertPath);

    std::string vertSrc((std::istreambuf_iterator<char>(vertFin)),
                        std::istreambuf_iterator<char>());

    std::ifstream fragFin(fragPath);
    if (!fragFin.is_open())
        throw std::runtime_error(
            std::string("Failed to load fragment shader from ") + fragPath);

    std::string fragSrc((std::istreambuf_iterator<char>(fragFin)),
                        std::istreambuf_iterator<char>());

    if (geomPath == nullptr)
        create(vertSrc.c_str(), fragSrc.c_str());
    else
    {
        std::ifstream geomFin(geomPath);
        if (!geomFin.is_open())
            throw std::runtime_error(
                std::string("Failed to load geometry shader from ") + geomPath);

        std::string geomSrc((std::istreambuf_iterator<char>(geomFin)),
                            std::istreambuf_iterator<char>());

        create(vertSrc.c_str(), fragSrc.c_str(), geomSrc.c_str());
    }
}

void         Shader::use() const { glUseProgram(m_id); }
unsigned int Shader::getId() const { return m_id; }
void         Shader::destroy()
{
    if (m_id != 0)
    {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}
Shader::~Shader() { destroy(); }

int Shader::getUniformLocation(const std::string &name) const
{
    auto iter = m_uniformLocationCache.find(name);
    if (iter == m_uniformLocationCache.end())
    {
        int loc = glGetUniformLocation(m_id, name.c_str());
        m_uniformLocationCache.insert({name, loc});
        return loc;
    }
    else
        return iter->second;
}

void Shader::setUniform1i(const std::string &name, int n) const
{
    glUniform1i(getUniformLocation(name), n);
}

void Shader::setUniform2fv(const std::string &name, const float *v) const
{
    glUniform2fv(getUniformLocation(name), 1, v);
}

void Shader::setUniform3fv(const std::string &name, const float *v) const
{
    glUniform3fv(getUniformLocation(name), 1, v);
}

void Shader::setUniform1f(const std::string &name, float f) const
{
    glUniform1f(getUniformLocation(name), f);
}

void Shader::setUniform4fv(const std::string &name, const float *v) const
{
    glUniform4fv(getUniformLocation(name), 1, v);
}

void Shader::setUniformMatrix4fv(const std::string &name, const float *ptr,
                                 bool transpose, unsigned int count) const
{
    glUniformMatrix4fv(getUniformLocation(name), count, transpose, ptr);
}

} // namespace ogl