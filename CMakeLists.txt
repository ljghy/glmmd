cmake_minimum_required(VERSION 3.12)

project(glmmd)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if(MSVC)
    ADD_DEFINITIONS(/MP)
endif()

option(GLMMD_BUILD_APPS "Build glmmd apps" ON)

set(GLMMD_BULLET_ROOT "" CACHE PATH "Bullet physics root directory")

if(NOT "${GLMMD_BULLET_ROOT}" STREQUAL "")
    set(BULLET_ROOT "${GLMMD_BULLET_ROOT}")
endif()

option(GLMMD_DO_NOT_USE_STD_EXECUTION "Don't use std::execution" OFF)
option(GLMMD_DO_NOT_FORCE_UTF8 "Don't convert names to UTF-8" OFF)

add_subdirectory(libs)
add_subdirectory(src)

if(GLMMD_BUILD_APPS)
    add_subdirectory(apps)
endif()
