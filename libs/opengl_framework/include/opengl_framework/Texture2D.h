#ifndef TEXTURE_2D_H_
#define TEXTURE_2D_H_

#include <glad/glad.h>

struct Texture2DCreateInfo
{
    int width;
    int height;

    const unsigned char *data = nullptr;

    GLenum internalFmt = GL_RGBA;
    GLenum dataFmt     = GL_RGBA;
    GLenum dataType    = GL_UNSIGNED_BYTE;

    int  mipmapLevel = 0;
    bool genMipmap   = false;

    GLenum wrapMode   = GL_REPEAT;
    GLenum filterMode = GL_LINEAR;
};

class Texture2D
{

public:
    Texture2D();
    ~Texture2D();

    Texture2D(const Texture2D &)            = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(Texture2D &&other);

    void create(const Texture2DCreateInfo &info);
    void destroy();

    void bind(unsigned int unit = 0) const;
    void unbind() const;

    int width() const { return m_info.width; }
    int height() const { return m_info.height; }

    unsigned int id() const { return m_id; }

private:
    unsigned int        m_id;
    Texture2DCreateInfo m_info;
};

#endif
