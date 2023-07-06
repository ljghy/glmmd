add_library(opengl_framework STATIC)

target_sources(opengl_framework
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VertexArrayObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VertexBufferObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IndexBufferObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture2D.cpp
)

target_include_directories(opengl_framework
PUBLIC
    ${PROJECT_SOURCE_DIR}/libs/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
