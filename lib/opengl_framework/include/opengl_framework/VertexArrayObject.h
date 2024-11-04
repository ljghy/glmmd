#ifndef OPENGL_VERTEX_ARRAY_OBJECT_H_
#define OPENGL_VERTEX_ARRAY_OBJECT_H_

#include <glad/glad.h>

#include <cassert>
#include <cstddef>
#include <vector>

#include <opengl_framework/APIConfig.h>
#include <opengl_framework/VertexBufferObject.h>

namespace ogl
{

class OPENGL_FRAMEWORK_API VertexBufferLayout
{
    enum Layout
    {
        AoS,
        SoA
    };

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

    VertexBufferLayout(unsigned int count = 0)
        : m_count(count)
        , m_type(count > 0 ? SoA : AoS)
    {
        if (m_type == AoS)
            m_strides.push_back(0);
        m_offsets.push_back(0);
    }

    unsigned int getElementCount() const
    {
        return static_cast<unsigned int>(m_elements.size());
    }
    unsigned int getStride(size_t i) const
    {
        return m_type == SoA ? m_strides[i] : m_strides[0];
    }

    unsigned int getType(size_t i) const { return m_elements[i].first; }
    unsigned int getCount(size_t i) const { return m_elements[i].second; }
    unsigned int getOffset(size_t i) const { return m_offsets[i]; }

    void push(unsigned int ty, unsigned int count)
    {
        m_elements.push_back({ty, count});
        if (m_type == SoA)
        {
            m_strides.push_back(getSize(ty) * count);
            m_offsets.push_back(m_offsets.back() +
                                getSize(ty) * count * m_count);
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
    unsigned int              m_count;
    Layout                    m_type;
};

class OPENGL_FRAMEWORK_API VertexArrayObject
{
public:
    VertexArrayObject();
    ~VertexArrayObject();

    VertexArrayObject(const VertexArrayObject &)            = delete;
    VertexArrayObject &operator=(const VertexArrayObject &) = delete;

    VertexArrayObject(VertexArrayObject &&other) noexcept;
    VertexArrayObject &operator=(VertexArrayObject &&other) noexcept;

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