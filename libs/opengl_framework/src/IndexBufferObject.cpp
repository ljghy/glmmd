#include <glad/glad.h>

#include <cassert>

#include <opengl_framework/IndexBufferObject.h>

IndexBufferObject::IndexBufferObject()
    : m_id(0)
{
}
IndexBufferObject::~IndexBufferObject() { destroy(); }

IndexBufferObject::IndexBufferObject(IndexBufferObject &&other)
    : m_id(other.m_id)
    , m_count(other.m_count)
{
    other.m_id = other.m_count = 0;
}

void IndexBufferObject::create(const unsigned int *data, unsigned int size,
                               GLenum drawType)
{
    destroy();
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, drawType);
    m_count = size / sizeof(unsigned int);
}

void IndexBufferObject::destroy()
{
    if (m_id != 0)
    {
        glDeleteBuffers(1, &m_id);
        m_id = 0;
    }
}

void IndexBufferObject::bind() const
{
    assert(m_id != 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void IndexBufferObject::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
