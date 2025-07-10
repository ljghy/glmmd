cmake_minimum_required(VERSION 3.20)

project(ImGuiFileDialog)

add_library(ImGuiFileDialog STATIC
    ImGuiFileDialog.cpp
    ImGuiFileDialog.h
    ImGuiFileDialogConfig.h
)

target_include_directories(ImGuiFileDialog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX)
    target_compile_options(ImGuiFileDialog PUBLIC -Wno-unknown-pragmas)
endif()

target_link_libraries(ImGuiFileDialog PUBLIC imgui)

target_compile_features(ImGuiFileDialog PUBLIC cxx_std_17)

target_compile_definitions(ImGuiFileDialog PRIVATE USE_STD_FILESYSTEM)
