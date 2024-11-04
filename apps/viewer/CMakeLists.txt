cmake_minimum_required(VERSION 3.14)

project(glmmd_viewer)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

option(GLMMD_APPS_USE_SYSTEM_GLFW "Use system glfw library" ON)
if(GLMMD_APPS_USE_SYSTEM_GLFW)
    find_package(glfw3)
endif()

if(NOT GLMMD_APPS_USE_SYSTEM_GLFW OR NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)

    set(GLFW_BUILD_DOCS
        OFF
        CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS
        OFF
        CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES
        OFF
        CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.4-docking)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC)

target_sources(
    imgui
    PRIVATE ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}
                                        ${imgui_SOURCE_DIR}/backends)

target_link_libraries(imgui PUBLIC glfw)

add_executable(viewer)

target_sources(
    viewer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AxesRenderer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/InfiniteGridRenderer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Context.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(viewer PRIVATE glmmd::glmmd imgui)
