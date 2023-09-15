file(GLOB_RECURSE SOURCES *.c *.cpp)

add_library(opengl_framework ${SOURCES})

target_include_directories(opengl_framework
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
