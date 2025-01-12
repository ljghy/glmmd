#ifndef TEXTURE_2D_H_
#define TEXTURE_2D_H_

#include <glad/glad.h>

#include <opengl_framework/APIConfig.h>
namespace ogl
{

struct OPENGL_FRAMEWORK_API Texture2DCreateInfo
{
    int width;
    int height;
    int samples = 1;

    const unsigned char *data = nullptr;

    bool genMipmaps = false;

    GLenum internalFmt   = GL_RGBA;
    GLenum dataFmt       = GL_RGBA;
    GLenum dataType      = GL_UNSIGNED_BYTE;
    GLenum wrapModeS     = GL_REPEAT;
    GLenum wrapModeT     = GL_REPEAT;
    GLenum minFilterMode = GL_LINEAR;
    GLenum magFilterMode = GL_LINEAR;
};

class OPENGL_FRAMEWORK_API Texture2D
{

public:
    Texture2D();
    Texture2D(const Texture2DCreateInfo &info);

    ~Texture2D();

    Texture2D(const Texture2D &)            = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(Texture2D &&other) noexcept;
    Texture2D &operator=(Texture2D &&other) noexcept;

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

    GLenum internalFormat() const { return m_info.internalFmt; }
    GLenum dataFormat() const { return m_info.dataFmt; }
    GLenum dataType() const { return m_info.dataType; }

private:
    unsigned int m_id;
    unsigned int m_target;

    Texture2DCreateInfo m_info;
};

} // namespace ogl

#endif
