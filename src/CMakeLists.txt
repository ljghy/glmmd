add_library(glmmd_compiler_flags INTERFACE)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(glmmd_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wshadow;-Wformat=2;-Wunused>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

file(GLOB_RECURSE GLMMD_SOURCES *.cpp)

add_library(glmmd_compile_definitions INTERFACE)

target_compile_definitions(glmmd_compile_definitions INTERFACE
    $<$<BOOL:${GLMMD_DO_NOT_USE_STD_EXECUTION}>:GLMMD_DO_NOT_USE_STD_EXECUTION>
    $<$<BOOL:${GLMMD_DO_NOT_FORCE_UTF8}>:GLMMD_DO_NOT_FORCE_UTF8>
)

add_library(glmmd ${GLMMD_SOURCES})
add_library(glmmd::glmmd ALIAS glmmd)

find_package(Bullet REQUIRED)
find_package(glm REQUIRED)

if ((NOT GLMMD_DO_NOT_USE_STD_EXECUTION) AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    find_package(TBB REQUIRED)
    set(GLMMD_TBB_LIBRARY TBB::tbb)
endif()

target_include_directories(glmmd
PUBLIC
    ${BULLET_INCLUDE_DIR}
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(glmmd
PUBLIC
    glmmd_compiler_flags
    glmmd_compile_definitions
    ${GLMMD_TBB_LIBRARY}
    glm::glm
    ${BULLET_LIBRARIES}
    opengl_framework::opengl_framework
    stb::stb
)
