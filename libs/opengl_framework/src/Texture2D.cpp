#include <opengl_framework/Texture2D.h>

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
    glGenTextures(1, &m_id);

    m_target = m_info.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    glBindTexture(m_target, m_id);

    if (m_target == GL_TEXTURE_2D_MULTISAMPLE)
    {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_info.samples,
                                m_info.internalFmt, m_info.width, m_info.height,
                                GL_TRUE);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0 /* mipmap level */, info.internalFmt,
                     info.width, info.height, 0, info.dataFmt, info.dataType,
                     info.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.filterMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.filterMode);
    }

    glBindTexture(m_target, 0);
}

void Texture2D::destroy()
{
    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

void Texture2D::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_id);
}

void Texture2D::unbind() const { glBindTexture(m_target, 0); }

void Texture2D::resize(int width, int height)
{
    m_info.width  = width;
    m_info.height = height;

    glBindTexture(m_target, m_id);

    if (m_target == GL_TEXTURE_2D_MULTISAMPLE)
    {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_info.samples,
                                m_info.internalFmt, m_info.width, m_info.height,
                                GL_TRUE);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, m_info.internalFmt, m_info.width,
                     m_info.height, 0, m_info.dataFmt, m_info.dataType,
                     m_info.data);
    }

    glBindTexture(m_target, 0);
}

} // namespace ogl
