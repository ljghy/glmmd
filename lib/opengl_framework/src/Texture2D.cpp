#include <opengl_framework/Texture2D.h>

#include "GLCheck.h"
namespace ogl
{

Texture2D::Texture2D()
    : m_id(0)
    , m_target(GL_TEXTURE_2D)
{
}

Texture2D::Texture2D(const Texture2DCreateInfo &info)
    : m_id(0)
    , m_target(GL_TEXTURE_2D)
    , m_info(info)
{
    create(info);
}

Texture2D::~Texture2D() { destroy(); }

Texture2D::Texture2D(Texture2D &&other)
    : m_id(other.m_id)
    , m_target(other.m_target)
    , m_info(other.m_info)
{
    other.m_id = 0;
}

void Texture2D::create(const Texture2DCreateInfo &info)
{
    destroy();
    m_info = info;
    GL_CHECK(glGenTextures(1, &m_id));

    m_target = m_info.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    GL_CHECK(glBindTexture(m_target, m_id));

    if (m_target == GL_TEXTURE_2D_MULTISAMPLE)
    {
        GL_CHECK(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                         m_info.samples, m_info.internalFmt,
                                         m_info.width, m_info.height, GL_TRUE));
    }
    else
    {
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, info.internalFmt, info.width,
                              info.height, 0, info.dataFmt, info.dataType,
                              info.data));
        if (info.genMipmaps)
            GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
        GL_CHECK(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrapModeS));
        GL_CHECK(
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrapModeT));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                 info.minFilterMode));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                 info.magFilterMode));
    }

    GL_CHECK(glBindTexture(m_target, 0));
}

void Texture2D::destroy()
{
    if (m_id != 0)
    {
        GL_CHECK(glDeleteTextures(1, &m_id));
        m_id = 0;
    }
}

void Texture2D::bind(unsigned int unit) const
{
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(m_target, m_id));
}

void Texture2D::unbind() const { GL_CHECK(glBindTexture(m_target, 0)); }

void Texture2D::resize(int width, int height)
{
    m_info.width  = width;
    m_info.height = height;

    GL_CHECK(glBindTexture(m_target, m_id));

    if (m_target == GL_TEXTURE_2D_MULTISAMPLE)
    {
        GL_CHECK(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                         m_info.samples, m_info.internalFmt,
                                         m_info.width, m_info.height, GL_TRUE));
    }
    else
    {
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, m_info.internalFmt,
                              m_info.width, m_info.height, 0, m_info.dataFmt,
                              m_info.dataType, m_info.data));
        if (m_info.genMipmaps)
            GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
    }

    GL_CHECK(glBindTexture(m_target, 0));
}

} // namespace ogl
