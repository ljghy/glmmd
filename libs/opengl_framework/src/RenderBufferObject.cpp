#include <glad/glad.h>
#include <opengl_framework/RenderBufferObject.h>

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

    glGenRenderbuffers(1, &m_id);
    glBindRenderbuffer(GL_RENDERBUFFER, m_id);

    if (m_info.samples > 1)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_info.samples,
                                         m_info.internalFmt, m_info.width,
                                         m_info.height);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, m_info.internalFmt, m_info.width,
                              m_info.height);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RenderBufferObject::destroy()
{
    if (m_id)
    {
        glDeleteRenderbuffers(1, &m_id);
        m_id = 0;
    }
}

void RenderBufferObject::bind() const
{
    glBindRenderbuffer(GL_RENDERBUFFER, m_id);
}

void RenderBufferObject::unbind() const
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RenderBufferObject::resize(int width, int height)
{
    m_info.width  = width;
    m_info.height = height;

    glBindRenderbuffer(GL_RENDERBUFFER, m_id);

    if (m_info.samples > 1)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_info.samples,
                                         m_info.internalFmt, width, height);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, m_info.internalFmt, width,
                              height);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}