cmake_minimum_required(VERSION 3.12)

project(stb LANGUAGES C VERSION 2.27)

if (MSVC)
    add_compile_options(/wd4996)
endif()

add_library(stb INTERFACE)
add_library(stb::stb ALIAS stb)

target_include_directories(stb
INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
