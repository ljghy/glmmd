#include <opengl_framework/VertexArrayObject.h>

VertexArrayObject::VertexArrayObject()
    : m_id(0)
{
}

VertexArrayObject::VertexArrayObject(VertexArrayObject &&other)
    : m_id(other.m_id)
{
    other.m_id = 0;
}

VertexArrayObject::~VertexArrayObject() { destroy(); }

void VertexArrayObject::create()
{
    destroy();
    glGenVertexArrays(1, &m_id);
}

void VertexArrayObject::addBuffer(const VertexBufferObject &vbo,
                                  const VertexBufferLayout &layout)
{
    bind();
    vbo.bind();

    for (unsigned int i = 0; i < layout.getElementCount(); ++i)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, layout.getCount(i), layout.getType(i),
                              GL_FALSE, layout.getStride(i),
                              (const void *)(uintptr_t)(layout.getOffset(i)));
    }
}

void VertexArrayObject::destroy()
{
    if (m_id != 0)
    {
        glDeleteVertexArrays(1, &m_id);
        m_id = 0;
    }
}

void VertexArrayObject::bind() const { glBindVertexArray(m_id); }

void VertexArrayObject::unbind() const { glBindVertexArray(0); }
