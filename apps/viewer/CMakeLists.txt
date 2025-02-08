cmake_minimum_required(VERSION 3.14)

project(glmmd_viewer)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(viewer)

target_sources(
    viewer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/InfiniteGridRenderer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ModelRenderer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Viewer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(
    viewer PRIVATE glmmd::glmmd imgui ImGuiFileDialog stb::stb glfw
                   opengl_framework::opengl_framework)

target_compile_features(viewer PRIVATE cxx_std_17)

set_target_properties(viewer PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                        ${CMAKE_BINARY_DIR}/bin)

add_custom_command(
    TARGET viewer
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/font
            $<TARGET_FILE_DIR:viewer>/font)
