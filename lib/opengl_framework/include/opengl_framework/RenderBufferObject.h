#ifndef OPENGL_RENDER_BUFFER_OBJECT_H_
#define OPENGL_RENDER_BUFFER_OBJECT_H_

namespace ogl
{

struct RenderBufferObjectCreateInfo
{
    int width;
    int height;
    int samples = 1;

    unsigned int internalFmt = GL_DEPTH24_STENCIL8;
};

class RenderBufferObject
{
public:
    RenderBufferObject();
    RenderBufferObject(const RenderBufferObjectCreateInfo &info);
    ~RenderBufferObject();

    RenderBufferObject(const RenderBufferObject &)            = delete;
    RenderBufferObject &operator=(const RenderBufferObject &) = delete;
    RenderBufferObject(RenderBufferObject &&);

    void create(const RenderBufferObjectCreateInfo &info);
    void destroy();

    void bind() const;
    void unbind() const;

    int width() const { return m_info.width; }
    int height() const { return m_info.height; }

    unsigned int id() const { return m_id; }

    void resize(int width, int height);

private:
    unsigned int m_id;

    RenderBufferObjectCreateInfo m_info;
};

} // namespace ogl

#endif
