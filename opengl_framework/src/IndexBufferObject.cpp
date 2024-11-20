#include <glad/glad.h>

#include <opengl_framework/IndexBufferObject.h>

#include "GLCheck.h"

namespace ogl
{

IndexBufferObject::IndexBufferObject()
    : m_id(0)
{
}
IndexBufferObject::~IndexBufferObject() { destroy(); }

IndexBufferObject::IndexBufferObject(IndexBufferObject &&other) noexcept
    : m_id(other.m_id)
    , m_count(other.m_count)
{
    other.m_id = other.m_count = 0;
}

IndexBufferObject &
IndexBufferObject::operator=(IndexBufferObject &&other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_id       = other.m_id;
        m_count    = other.m_count;
        other.m_id = other.m_count = 0;
    }
    return *this;
}

void IndexBufferObject::create(const unsigned int *data, unsigned int size,
                               GLenum drawType)
{
    destroy();
    GL_CHECK(glGenBuffers(1, &m_id));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, drawType));
    m_count = size / sizeof(unsigned int);
}

void IndexBufferObject::destroy()
{
    if (m_id != 0)
    {
        GL_CHECK(glDeleteBuffers(1, &m_id));
        m_id = 0;
    }
}

void IndexBufferObject::bind() const
{
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id));
}

void IndexBufferObject::unbind() const
{
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

} // namespace ogl
