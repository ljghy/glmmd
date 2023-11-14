cmake_minimum_required(VERSION 3.12)

project(stb LANGUAGES C VERSION 2.27)

if (MSVC)
    add_compile_options(/wd4996)
endif()

add_library(stb INTERFACE)
add_library(stb::stb ALIAS stb)

target_include_directories(stb
INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/stb-${stb_VERSION}>
)

install(TARGETS stb EXPORT stbTargets)

install(DIRECTORY include/ DESTINATION include/stb-${stb_VERSION})

install(EXPORT stbTargets
        FILE stbTargets.cmake
        NAMESPACE stb::
        DESTINATION lib/cmake/stb-${stb_VERSION}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stbConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/stbConfig.cmake"
    INSTALL_DESTINATION lib/cmake/stb-${stb_VERSION}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/stbConfigVersion.cmake"
    VERSION ${stb_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/stbConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/stbConfigVersion.cmake"
    DESTINATION lib/cmake/stb-${stb_VERSION}
)

export(EXPORT stbTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/stbTargets.cmake"
    NAMESPACE stb::
)
