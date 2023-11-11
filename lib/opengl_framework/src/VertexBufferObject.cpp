#include <glad/glad.h>
#include <opengl_framework/VertexBufferObject.h>

#include "GLCheck.h"
namespace ogl
{

VertexBufferObject::VertexBufferObject()
    : m_id(0)
{
}
VertexBufferObject::~VertexBufferObject() { destroy(); }
VertexBufferObject::VertexBufferObject(VertexBufferObject &&other)
    : m_id(other.m_id)
{
    other.m_id = 0;
}

void VertexBufferObject::create(const void *data, unsigned int size,
                                GLenum drawType)
{
    destroy();
    GL_CHECK(glGenBuffers(1, &m_id));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_id));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, size, data, drawType));
}

void VertexBufferObject::destroy()
{
    if (m_id != 0)
    {
        GL_CHECK(glDeleteBuffers(1, &m_id));
        m_id = 0;
    }
}

void VertexBufferObject::bind() const
{
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_id));
}

void VertexBufferObject::unbind() const
{
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBufferObject::uploadSubData(const void* data, unsigned int offset,
                                       unsigned int size)
{
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_id));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

} // namespace ogl
