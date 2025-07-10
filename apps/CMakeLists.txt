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
    GIT_TAG v1.92.0-docking)
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

add_subdirectory(ImGuiFileDialog)
add_subdirectory(stb)
add_subdirectory(opengl_framework)
add_subdirectory(viewer)
