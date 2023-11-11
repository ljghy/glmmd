#ifndef OPENGL_FRAME_BUFFER_OBJECT_H_
#define OPENGL_FRAME_BUFFER_OBJECT_H_

#include <memory>

#include <opengl_framework/APIConfig.h>
#include <opengl_framework/Texture2D.h>
#include <opengl_framework/RenderBufferObject.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API FrameBufferObject
{
public:
    FrameBufferObject();
    ~FrameBufferObject();

    FrameBufferObject(const FrameBufferObject &)            = delete;
    FrameBufferObject &operator=(const FrameBufferObject &) = delete;
    FrameBufferObject(FrameBufferObject &&);

    void create();
    void destroy();

    void bind() const;
    void unbind() const;
    void bindRead() const;
    void bindDraw() const;

    void attachColorTexture(std::unique_ptr<Texture2D> &&texture);
    void attachDepthTexture(std::unique_ptr<Texture2D> &&texture);
    void
    attachDepthRenderBuffer(std::unique_ptr<RenderBufferObject> &&renderBuffer);

    bool isComplete() const;

    unsigned int id() const { return m_id; }

    const Texture2D *colorTextureAttachment() const
    {
        return m_colorTextureAttachment.get();
    }

    const Texture2D *depthTextureAttachment() const
    {
        return m_depthTextureAttachment.get();
    }

    const RenderBufferObject *depthRenderBufferAttachment() const
    {
        return m_depthRenderBufferAttachment.get();
    }

    void resize(int width, int height);

    void readColorAttachment(void *buffer, GLenum format = GL_RGBA,
                             GLenum type = GL_UNSIGNED_BYTE, int x = 0,
                             int y = 0, int w = -1, int h = -1) const;

private:
    unsigned int m_id;

    std::unique_ptr<Texture2D>          m_colorTextureAttachment      = nullptr;
    std::unique_ptr<Texture2D>          m_depthTextureAttachment      = nullptr;
    std::unique_ptr<RenderBufferObject> m_depthRenderBufferAttachment = nullptr;
};

} // namespace ogl

#endif
