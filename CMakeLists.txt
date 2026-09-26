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
option(GLMMD_USE_BULLET "Use Bullet physics engine" ON)
option(GLMMD_USE_ICU "Use libicu" OFF)
option(GLMMD_INSTALL "Generate glmmd install rules" ${GLMMD_IS_TOPLEVEL})

find_package(glm REQUIRED)

include(GNUInstallDirs)

add_subdirectory(src)

if(GLMMD_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(GLMMD_INSTALL)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS glmmd glmmd_core glmmd_files glmmd_config
        EXPORT glmmdTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/glmmd
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(
        EXPORT glmmdTargets
        FILE glmmdTargets.cmake
        NAMESPACE glmmd::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glmmd)

    configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/glmmdConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/glmmdConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glmmd)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/glmmdConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/glmmdConfig.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/glmmdConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glmmd)
endif()
