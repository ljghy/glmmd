add_library(glmmd_compiler_flags INTERFACE)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(glmmd_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

file(GLOB_RECURSE GLMMD_SOURCES *.cpp)

add_library(glmmd ${GLMMD_SOURCES})

find_package(Bullet REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

find_package(glm)
if (NOT glm_FOUND)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

target_compile_definitions(glmmd
PUBLIC
    $<$<BOOL:${GLMMD_DO_NOT_USE_STD_EXECUTION}>:GLMMD_DO_NOT_USE_STD_EXECUTION>
    $<$<BOOL:${GLMMD_DO_NOT_FORCE_UTF8}>:GLMMD_DO_NOT_FORCE_UTF8>
)

target_include_directories(glmmd
PUBLIC
    ${BULLET_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

if ((NOT GLMMD_DO_NOT_USE_STD_EXECUTION) AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    find_package(TBB REQUIRED)
    set(GLMMD_TBB_LIBRARY "TBB::tbb")
endif()

target_link_libraries(glmmd
PRIVATE
    glmmd_compiler_flags
    ${GLMMD_TBB_LIBRARY}
PUBLIC
    glm::glm
    ${BULLET_LIBRARIES}
    opengl_framework::opengl_framework
    stb::stb
)
