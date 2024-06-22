get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(GLMMD_IS_TOPLEVEL FALSE)
else()
    set(GLMMD_IS_TOPLEVEL TRUE)
endif()

if(GLMMD_IS_TOPLEVEL)
    cmake_minimum_required(VERSION 3.14)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE
            "Release"
            CACHE STRING "" FORCE)
    endif()
endif()

project(
    glmmd
    LANGUAGES CXX
    VERSION 0.0.1)

if(MSVC)
    add_compile_options(/MP /wd4996 /Zc:__cplusplus)
endif()

option(GLMMD_DO_NOT_USE_STD_EXECUTION "Do not use std::execution" OFF)
option(GLMMD_DO_NOT_FORCE_UTF8 "Do not convert names to UTF-8" OFF)
option(GLMMD_BUILD_APPS "Build glmmd apps" ${GLMMD_IS_TOPLEVEL})
option(GLMMD_BUILD_RENDERER "Build renderer" ON)

set(GLMMD_BULLET_ROOT
    ""
    CACHE PATH "Bullet physics root directory")
if(NOT "${GLMMD_BULLET_ROOT}" STREQUAL "")
    set(BULLET_ROOT "${GLMMD_BULLET_ROOT}")
endif()

add_subdirectory(lib)
add_subdirectory(src)

if(GLMMD_BUILD_APPS)
    add_subdirectory(apps)
endif()
