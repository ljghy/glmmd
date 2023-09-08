#ifndef TEXTURE_2D_H_
#define TEXTURE_2D_H_

#include <glad/glad.h>

struct Texture2DCreateInfo
{
    int width;
    int height;
    int samples = 1;

    const unsigned char *data = nullptr;

    GLenum internalFmt = GL_RGBA;
    GLenum dataFmt     = GL_RGBA;
    GLenum dataType    = GL_UNSIGNED_BYTE;
    GLenum wrapMode    = GL_REPEAT;
    GLenum filterMode  = GL_LINEAR;
};

class Texture2D
{

public:
    Texture2D();
    Texture2D(const Texture2DCreateInfo &info);

    ~Texture2D();

    Texture2D(const Texture2D &)            = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(Texture2D &&other);

    void create(const Texture2DCreateInfo &info);
    void destroy();

    void bind(unsigned int unit = 0) const;
    void unbind() const;

    int          width() const { return m_info.width; }
    int          height() const { return m_info.height; }
    int          samples() const { return m_info.samples; }
    unsigned int target() const { return m_target; }

    unsigned int id() const { return m_id; }

    void resize(int width, int height);

private:
    unsigned int m_id;
    unsigned int m_target;

    Texture2DCreateInfo m_info;
};

#endif
