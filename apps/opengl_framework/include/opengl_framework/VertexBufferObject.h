#ifndef OPENGL_VERTEX_BUFFER_OBJECT_H_
#define OPENGL_VERTEX_BUFFER_OBJECT_H_

#include <glad/glad.h>

#include <opengl_framework/APIConfig.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API VertexBufferObject
{
public:
    VertexBufferObject();
    ~VertexBufferObject();

    VertexBufferObject(const VertexBufferObject &)            = delete;
    VertexBufferObject &operator=(const VertexBufferObject &) = delete;
    VertexBufferObject(VertexBufferObject &&) noexcept;
    VertexBufferObject &operator=(VertexBufferObject &&) noexcept;

    void create(const void *data, unsigned int size,
                GLenum drawType = GL_STATIC_DRAW);
    void destroy();

    void bind() const;
    void unbind() const;

    void uploadSubData(const void *data, unsigned int offset,
                       unsigned int size);

private:
    unsigned int m_id;
};

} // namespace ogl

#endif
