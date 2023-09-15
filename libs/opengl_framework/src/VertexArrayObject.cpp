#include <opengl_framework/VertexArrayObject.h>

#include "GLCheck.h"
namespace ogl
{

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
    GL_CHECK(glGenVertexArrays(1, &m_id));
}

void VertexArrayObject::addBuffer(const VertexBufferObject &vbo,
                                  const VertexBufferLayout &layout)
{
    bind();
    vbo.bind();

    for (unsigned int i = 0; i < layout.getElementCount(); ++i)
    {
        GL_CHECK(glEnableVertexAttribArray(i));
        GL_CHECK(glVertexAttribPointer(
            i, layout.getCount(i), layout.getType(i), GL_FALSE,
            layout.getStride(i),
            (const void *)(uintptr_t)(layout.getOffset(i))));
    }
}

void VertexArrayObject::destroy()
{
    if (m_id != 0)
    {
        GL_CHECK(glDeleteVertexArrays(1, &m_id));
        m_id = 0;
    }
}

void VertexArrayObject::bind() const { GL_CHECK(glBindVertexArray(m_id)); }

void VertexArrayObject::unbind() const { GL_CHECK(glBindVertexArray(0)); }

} // namespace ogl
