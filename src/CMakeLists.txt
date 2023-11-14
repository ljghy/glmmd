add_library(glmmd_compiler_flags INTERFACE)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(glmmd_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wshadow;-Wformat=2;-Wunused>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3 /MP>>"
)

file(GLOB_RECURSE GLMMD_SOURCES *.cpp)

add_library(glmmd_compile_definitions INTERFACE)

target_compile_definitions(glmmd_compile_definitions INTERFACE
    $<$<BOOL:${GLMMD_DO_NOT_USE_STD_EXECUTION}>:GLMMD_DO_NOT_USE_STD_EXECUTION>
    $<$<BOOL:${GLMMD_DO_NOT_FORCE_UTF8}>:GLMMD_DO_NOT_FORCE_UTF8>
    $<INSTALL_INTERFACE:GLMMD_EXPORT>
    $<$<BOOL:${BUILD_SHARED_LIBS}>:GLMMD_BUILD_SHARED_LIBS>
)

add_library(glmmd ${GLMMD_SOURCES})
add_library(glmmd::glmmd ALIAS glmmd)

set_target_properties(glmmd
PROPERTIES
    DEBUG_POSTFIX d
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/glmmd-${glmmd_VERSION}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

find_package(Bullet REQUIRED)
find_package(glm REQUIRED)

if ((NOT GLMMD_DO_NOT_USE_STD_EXECUTION) AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    find_package(TBB REQUIRED)
    set(GLMMD_TBB_LIBRARY TBB::tbb)
endif()

target_include_directories(glmmd
PUBLIC
    ${BULLET_INCLUDE_DIR}
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/glmmd-${glmmd_VERSION}>
)

target_link_libraries(glmmd
PUBLIC
    glmmd_compiler_flags
    glmmd_compile_definitions
    ${GLMMD_TBB_LIBRARY}
    glm::glm
    ${BULLET_LIBRARIES}
    opengl_framework::opengl_framework
    stb::stb
)

install(TARGETS glmmd glmmd_compiler_flags glmmd_compile_definitions
    EXPORT glmmdTargets
    DESTINATION lib/glmmd-${glmmd_VERSION}
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION include/glmmd-${glmmd_VERSION}
)

install(EXPORT glmmdTargets
    FILE glmmdTargets.cmake
    NAMESPACE glmmd::
    DESTINATION lib/cmake/glmmd-${glmmd_VERSION}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/glmmdConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/glmmdConfig.cmake"
    INSTALL_DESTINATION lib/cmake/glmmd-${glmmd_VERSION}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/glmmdConfigVersion.cmake"
    VERSION ${glmmd_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/glmmdConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/glmmdConfigVersion.cmake"
    DESTINATION lib/cmake/glmmd-${glmmd_VERSION}
)

export(EXPORT glmmdTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/glmmdTargets.cmake"
    NAMESPACE glmmd::
)
