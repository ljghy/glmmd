#include <glad/glad.h>
#include <opengl_framework/FrameBufferObject.h>

#include "GLCheck.h"
namespace ogl
{

FrameBufferObject::FrameBufferObject()
    : m_id(0)
{
}

FrameBufferObject::~FrameBufferObject() { destroy(); }

FrameBufferObject::FrameBufferObject(FrameBufferObject &&other) noexcept
    : m_id(other.m_id)
    , m_colorTextureAttachment(std::move(other.m_colorTextureAttachment))
    , m_depthTextureAttachment(std::move(other.m_depthTextureAttachment))
    , m_depthRenderBufferAttachment(
          std::move(other.m_depthRenderBufferAttachment))
{
    other.m_id = 0;
}

FrameBufferObject &
FrameBufferObject::operator=(FrameBufferObject &&other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_id                     = other.m_id;
        m_colorTextureAttachment = std::move(other.m_colorTextureAttachment);
        m_depthTextureAttachment = std::move(other.m_depthTextureAttachment);
        m_depthRenderBufferAttachment =
            std::move(other.m_depthRenderBufferAttachment);
        other.m_id = 0;
    }
    return *this;
}

void FrameBufferObject::create()
{
    destroy();
    GL_CHECK(glGenFramebuffers(1, &m_id));
}

void FrameBufferObject::destroy()
{
    if (m_id)
    {
        if (m_colorTextureAttachment)
            m_colorTextureAttachment->destroy();
        if (m_depthTextureAttachment)
            m_depthTextureAttachment->destroy();
        if (m_depthRenderBufferAttachment)
            m_depthRenderBufferAttachment->destroy();

        GL_CHECK(glDeleteFramebuffers(1, &m_id));
        m_id = 0;
    }
}

void FrameBufferObject::bind() const
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_id));
}

void FrameBufferObject::unbind() const
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBufferObject::bindRead() const
{
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id));
}

void FrameBufferObject::bindDraw() const
{
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id));
}

void FrameBufferObject::attachColorTexture(std::unique_ptr<Texture2D> &&texture)
{
    bind();
    m_colorTextureAttachment = std::move(texture);
    m_colorTextureAttachment->bind();
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    m_colorTextureAttachment->target(),
                                    m_colorTextureAttachment->id(), 0));
    unbind();
}

void FrameBufferObject::attachDepthTexture(std::unique_ptr<Texture2D> &&texture)
{
    bind();
    m_depthTextureAttachment = std::move(texture);
    m_depthTextureAttachment->bind();
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    m_depthTextureAttachment->target(),
                                    m_depthTextureAttachment->id(), 0));
    unbind();
}

void FrameBufferObject::attachDepthRenderBuffer(
    std::unique_ptr<RenderBufferObject> &&renderBuffer)
{
    bind();
    m_depthRenderBufferAttachment = std::move(renderBuffer);
    m_depthRenderBufferAttachment->bind();
    GL_CHECK(glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
        m_depthRenderBufferAttachment->id()));
    unbind();
}

bool FrameBufferObject::isComplete() const
{
    bind();
    bool ret =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    unbind();
    return ret;
}

void FrameBufferObject::resize(int width, int height)
{
    if (m_colorTextureAttachment)
    {
        m_colorTextureAttachment->resize(width, height);
    }
    if (m_depthTextureAttachment)
    {
        m_depthTextureAttachment->resize(width, height);
    }
    if (m_depthRenderBufferAttachment)
    {
        m_depthRenderBufferAttachment->resize(width, height);
    }
}

void FrameBufferObject::readColorAttachment(void *buffer, GLenum format,
                                            GLenum type, int x, int y, int w,
                                            int h) const
{
    if (!m_colorTextureAttachment)
        return;

    if (w < 0)
        w = m_colorTextureAttachment->width();
    if (h < 0)
        h = m_colorTextureAttachment->height();

    bindRead();
    GL_CHECK(glReadPixels(x, y, w, h, format, type, buffer));
    unbind();
}

} // namespace ogl
