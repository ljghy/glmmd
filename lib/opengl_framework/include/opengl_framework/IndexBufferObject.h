#ifndef OPENGL_INDEX_BUFFER_OBJECT_H_
#define OPENGL_INDEX_BUFFER_OBJECT_H_

#include <glad/glad.h>

#include <opengl_framework/APIConfig.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API IndexBufferObject
{
public:
    IndexBufferObject();
    ~IndexBufferObject();

    IndexBufferObject(const IndexBufferObject &)            = delete;
    IndexBufferObject &operator=(const IndexBufferObject &) = delete;
    IndexBufferObject(IndexBufferObject &&) noexcept;
    IndexBufferObject &operator=(IndexBufferObject &&) noexcept;

    void                create(const unsigned int *data, unsigned int size,
                               GLenum drawType = GL_STATIC_DRAW);
    void                destroy();
    inline unsigned int getCount() const { return m_count; }

    void bind() const;
    void unbind() const;

private:
    unsigned int m_id;
    unsigned int m_count;
};

} // namespace ogl

#endif