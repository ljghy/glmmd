cmake_minimum_required(VERSION 3.12)

project(
    stbi
    LANGUAGES C
    VERSION 2.30)

add_library(stbi src/stb_image_impl.c src/stb_image_write_impl.c)
add_library(stbi::stbi ALIAS stbi)

target_include_directories(stbi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
