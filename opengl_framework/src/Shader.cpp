#include <glad/glad.h>

#include <string>
#include <stdexcept>
#include <fstream>

#include <opengl_framework/Shader.h>

#include "GLCheck.h"

namespace ogl
{

Shader::Shader()
    : m_id(0)
{
}

Shader::Shader(Shader &&other) noexcept
    : m_id(other.m_id)
{
    other.m_id = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_id       = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

void Shader::create(const char *vertSrc, const char *fragSrc,
                    const char *geomSrc)
{
    destroy();

    unsigned int vert, frag, geometry{};
    int          success;
    char         infoLog[512];

    GL_CHECK(vert = glCreateShader(GL_VERTEX_SHADER));
    GL_CHECK(glShaderSource(vert, 1, &vertSrc, NULL));
    GL_CHECK(glCompileShader(vert));
    GL_CHECK(glGetShaderiv(vert, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        GL_CHECK(glGetShaderInfoLog(vert, 512, NULL, infoLog));
        throw std::runtime_error(std::string("Vertex shader compile error: ") +
                                 infoLog);
    };

    GL_CHECK(frag = glCreateShader(GL_FRAGMENT_SHADER));
    GL_CHECK(glShaderSource(frag, 1, &fragSrc, NULL));
    GL_CHECK(glCompileShader(frag));
    GL_CHECK(glGetShaderiv(frag, GL_COMPILE_STATUS, &success));
    if (!success)
    {
        GL_CHECK(glGetShaderInfoLog(frag, 512, NULL, infoLog));
        throw std::runtime_error(
            std::string("Fragment shader compile error: ") + infoLog);
    };

    if (geomSrc)
    {
        GL_CHECK(geometry = glCreateShader(GL_GEOMETRY_SHADER));
        GL_CHECK(glShaderSource(geometry, 1, &geomSrc, NULL));
        GL_CHECK(glCompileShader(geometry));
        GL_CHECK(glGetShaderiv(geometry, GL_COMPILE_STATUS, &success));
        if (!success)
        {
            GL_CHECK(glGetShaderInfoLog(geometry, 512, NULL, infoLog));
            throw std::runtime_error(
                std::string("Geometry shader compile error: ") + infoLog);
        };
    }

    GL_CHECK(m_id = glCreateProgram());
    GL_CHECK(glAttachShader(m_id, vert));
    GL_CHECK(glAttachShader(m_id, frag));
    if (geomSrc)
        GL_CHECK(glAttachShader(m_id, geometry));
    GL_CHECK(glLinkProgram(m_id));

    GL_CHECK(glGetProgramiv(m_id, GL_LINK_STATUS, &success));
    if (!success)
    {
        GL_CHECK(glGetProgramInfoLog(m_id, 512, NULL, infoLog));
        throw std::runtime_error(std::string("Shader link error: ") + infoLog);
    }

    GL_CHECK(glDeleteShader(vert));
    GL_CHECK(glDeleteShader(frag));
    if (geomSrc)
        GL_CHECK(glDeleteShader(geometry));
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

void         Shader::use() const { GL_CHECK(glUseProgram(m_id)); }
unsigned int Shader::getId() const { return m_id; }
void         Shader::destroy()
{
    if (m_id != 0)
    {
        GL_CHECK(glDeleteProgram(m_id));
        m_id = 0;
    }
}
Shader::~Shader() { destroy(); }

int Shader::getUniformLocation(const std::string &name) const
{
    auto iter = m_uniformLocationCache.find(name);
    if (iter == m_uniformLocationCache.end())
    {
        int loc;
        GL_CHECK(loc = glGetUniformLocation(m_id, name.c_str()));
        m_uniformLocationCache.insert({name, loc});
        return loc;
    }
    else
        return iter->second;
}

void Shader::setUniform1i(const std::string &name, int n) const
{
    GL_CHECK(glUniform1i(getUniformLocation(name), n));
}

void Shader::setUniform2fv(const std::string &name, const float *v) const
{
    GL_CHECK(glUniform2fv(getUniformLocation(name), 1, v));
}

void Shader::setUniform3fv(const std::string &name, const float *v) const
{
    GL_CHECK(glUniform3fv(getUniformLocation(name), 1, v));
}

void Shader::setUniform1f(const std::string &name, float f) const
{
    GL_CHECK(glUniform1f(getUniformLocation(name), f));
}

void Shader::setUniform4fv(const std::string &name, const float *v) const
{
    GL_CHECK(glUniform4fv(getUniformLocation(name), 1, v));
}

void Shader::setUniformMatrix4fv(const std::string &name, const float *ptr,
                                 bool transpose, unsigned int count) const
{
    GL_CHECK(
        glUniformMatrix4fv(getUniformLocation(name), count, transpose, ptr));
}

} // namespace ogl