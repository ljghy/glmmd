file(GLOB GLMMD_RENDER_SOURCES *.cpp)
add_library(glmmd_render ${GLMMD_RENDER_SOURCES})
add_library(glmmd::render ALIAS glmmd_render)

target_include_directories(glmmd_render PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(glmmd_render PUBLIC glmmd_core
                                          opengl_framework::opengl_framework)
