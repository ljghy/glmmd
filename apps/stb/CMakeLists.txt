cmake_minimum_required(VERSION 3.12)

project(
    stb
    LANGUAGES C
    VERSION 2.30)

add_library(stb src/stb_image_impl.c src/stb_image_write_impl.c)
add_library(stb::stb ALIAS stb)

target_include_directories(stb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
