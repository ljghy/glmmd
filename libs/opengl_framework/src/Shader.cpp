#include <glad/glad.h>

#include <string>
#include <stdexcept>

#include <opengl_framework/Shader.h>

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
                    const char *geometrySrc)
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

    if (geometrySrc)
    {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geometrySrc, NULL);
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
    if (geometrySrc)
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
    if (geometrySrc)
        glDeleteShader(geometry);
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

int Shader::getUniformLocation(const std::string &name)
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

void Shader::setUniform1i(const std::string &name, int n)
{
    glUniform1i(getUniformLocation(name), n);
}

void Shader::setUniform2fv(const std::string &name, const float *v)
{
    glUniform2fv(getUniformLocation(name), 1, v);
}

void Shader::setUniform3fv(const std::string &name, const float *v)
{
    glUniform3fv(getUniformLocation(name), 1, v);
}

void Shader::setUniform1f(const std::string &name, float f)
{
    glUniform1f(getUniformLocation(name), f);
}

void Shader::setUniform4fv(const std::string &name, const float *v)
{
    glUniform4fv(getUniformLocation(name), 1, v);
}

void Shader::setUniformMatrix4fv(const std::string &name, const float *ptr,
                                 bool transpose, unsigned int count)
{
    glUniformMatrix4fv(getUniformLocation(name), count, transpose, ptr);
}