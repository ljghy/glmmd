#include <glad/glad.h>
#include <opengl_framework/RenderBufferObject.h>

#include "GLCheck.h"
namespace ogl
{

RenderBufferObject::RenderBufferObject()
    : m_id(0)
{
}

RenderBufferObject::RenderBufferObject(const RenderBufferObjectCreateInfo &info)
    : m_id(0)
    , m_info(info)
{
    create(info);
}

RenderBufferObject::~RenderBufferObject() { destroy(); }

RenderBufferObject::RenderBufferObject(RenderBufferObject &&other)
    : m_id(other.m_id)
{
    other.m_id = 0;
}

void RenderBufferObject::create(const RenderBufferObjectCreateInfo &info)
{
    destroy();

    m_info = info;

    GL_CHECK(glGenRenderbuffers(1, &m_id));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_id));

    if (m_info.samples > 1)
    {
        GL_CHECK(glRenderbufferStorageMultisample(
            GL_RENDERBUFFER, m_info.samples, m_info.internalFmt, m_info.width,
            m_info.height));
    }
    else
    {
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, m_info.internalFmt,
                                       m_info.width, m_info.height));
    }

    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void RenderBufferObject::destroy()
{
    if (m_id)
    {
        GL_CHECK(glDeleteRenderbuffers(1, &m_id));
        m_id = 0;
    }
}

void RenderBufferObject::bind() const
{
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_id));
}

void RenderBufferObject::unbind() const
{
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void RenderBufferObject::resize(int width, int height)
{
    m_info.width  = width;
    m_info.height = height;

    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_id));

    if (m_info.samples > 1)
    {
        GL_CHECK(glRenderbufferStorageMultisample(
            GL_RENDERBUFFER, m_info.samples, m_info.internalFmt, width,
            height));
    }
    else
    {
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, m_info.internalFmt,
                                       width, height));
    }

    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

} // namespace ogl
