#ifndef OPENGL_CHECK_H_
#define OPENGL_CHECK_H_

#if defined(NDEBUG) && !defined(GL_ENABLE_DEBUG)
#define GL_CHECK(stmt) stmt
#else
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#define GL_CHECK(stmt)                                                         \
    do                                                                         \
    {                                                                          \
        stmt;                                                                  \
        GLenum err = glGetError();                                             \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            fprintf(stderr, "OpenGL error %d at %s:%d, %s.\n", err, __FILE__,  \
                    __LINE__, __FUNCTION__);                                   \
            assert(0);                                                         \
        }                                                                      \
    } while (0)
#endif

#endif
