#include <opengl_framework/Texture2D.h>

Texture2D::Texture2D()
    : m_id(0)
{
}
Texture2D::~Texture2D() { destroy(); }
Texture2D::Texture2D(Texture2D &&other)
    : m_id(other.m_id)
    , m_info(other.m_info)
{
    other.m_id = 0;
}

void Texture2D::create(const Texture2DCreateInfo &info)
{
    destroy();
    m_info = info;
    glGenTextures(1, &m_id);

    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, info.mipmapLevel, info.internalFmt, info.width,
                 info.height, 0, info.dataFmt, info.dataType, info.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.filterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.filterMode);
    if (info.genMipmap)
        glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2D::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }
