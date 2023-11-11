#ifndef OPENGL_FRAMEWORK_API_CONFIG_H_
#define OPENGL_FRAMEWORK_API_CONFIG_H_

#if defined(_WIN32) && defined(OPENGL_FRAMEWORK_EXPORTS_BUILD_DLL)
#ifdef OPENGL_FRAMEWORK_EXPORTS
#define OPENGL_FRAMEWORK_API __declspec(dllexport)
#else
#define OPENGL_FRAMEWORK_API __declspec(dllimport)
#endif
#else
#define OPENGL_FRAMEWORK_API
#endif

#endif
