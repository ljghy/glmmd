#ifndef OPENGL_VERTEX_ARRAY_OBJECT_H_
#define OPENGL_VERTEX_ARRAY_OBJECT_H_

#include <glad/glad.h>

#include <cstddef>
#include <vector>
#include <cassert>

#include <opengl_framework/APIConfig.h>
#include <opengl_framework/VertexBufferObject.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API VertexBufferLayout
{
public:
    static unsigned int getSize(unsigned int ty)
    {
        switch (ty)
        {
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
            return 4;
        case GL_UNSIGNED_BYTE:
            return 1;
        default:
            assert(0);
        }
        return 0;
    }

    VertexBufferLayout(bool useSubData = false)
        : m_useSubData(useSubData)
    {
        if (!useSubData)
            m_strides.push_back(0);
        m_offsets.push_back(0);
    }

    unsigned int getElementCount() const
    {
        return static_cast<unsigned int>(m_elements.size());
    }
    unsigned int getStride(size_t i) const
    {
        return m_useSubData ? m_strides[i] : m_strides[0];
    }

    unsigned int getType(size_t i) const { return m_elements[i].first; }
    unsigned int getCount(size_t i) const { return m_elements[i].second; }
    unsigned int getOffset(size_t i) const { return m_offsets[i]; }

    void push(unsigned int ty, unsigned int count, unsigned int elemCount = 0)
    {
        m_elements.push_back({ty, count});
        if (m_useSubData)
        {
            m_strides.push_back(getSize(ty) * count);
            m_offsets.push_back(m_offsets.back() + getSize(ty) * elemCount);
        }
        else
        {
            m_strides[0] += getSize(ty) * count;
            m_offsets.push_back(m_offsets.back() + getSize(ty) * count);
        }
    }

private:
    std::vector<std::pair<unsigned int, unsigned int>>
                              m_elements; // {type, count}
    std::vector<unsigned int> m_strides;
    std::vector<unsigned int> m_offsets;
    bool                      m_useSubData;
};

class OPENGL_FRAMEWORK_API VertexArrayObject
{
public:
    VertexArrayObject();
    ~VertexArrayObject();

    VertexArrayObject(const VertexArrayObject &)            = delete;
    VertexArrayObject &operator=(const VertexArrayObject &) = delete;

    VertexArrayObject(VertexArrayObject &&other);

    void create();
    void addBuffer(const VertexBufferObject &vbo,
                   const VertexBufferLayout &layout);
    void destroy();

    void bind() const;
    void unbind() const;

private:
    unsigned int m_id;
};

} // namespace ogl

#endif