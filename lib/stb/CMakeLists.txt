cmake_minimum_required(VERSION 3.12)

project(stb)

add_library(stb INTERFACE)
add_library(stb::stb ALIAS stb)

target_include_directories(stb INTERFACE include)
