#ifndef OPENGL_CHECK_H_
#define OPENGL_CHECK_H_

#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>

#if defined(NDEBUG) && !defined(GL_ENABLE_DEBUG)
#define GL_CHECK(stmt) stmt
#else
#define GL_CHECK(stmt)                                                         \
    do                                                                         \
    {                                                                          \
        stmt;                                                                  \
        GLenum err = glGetError();                                             \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            fprintf(stderr, "OpenGL error %d at %s:%d, %s.", err, __FILE__,    \
                    __LINE__, __FUNCTION__);                                   \
            assert(0);                                                         \
        }                                                                      \
    } while (0)
#endif

#endif
