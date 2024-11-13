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

option(GLMMD_DONT_PARALLELIZE "Do not parallelize" OFF)
option(GLMMD_BUILD_APPS "Build glmmd apps" ${GLMMD_IS_TOPLEVEL})
option(GLMMD_BUILD_RENDERER "Build renderer" ON)
option(GLMMD_USE_ICU "Use libicu" OFF)

add_subdirectory(lib)
add_subdirectory(src)

if(GLMMD_BUILD_APPS)
    add_subdirectory(apps)
endif()
