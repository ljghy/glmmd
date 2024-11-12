add_library(glmmd_compiler_flags INTERFACE)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(
    glmmd_compiler_flags
    INTERFACE
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wshadow;-Wformat=2;-Wunused>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3;/MP;/wd4996;/Zc:__cplusplus>>")

target_compile_features(glmmd_compiler_flags INTERFACE cxx_std_17)

add_library(glmmd_compile_definitions INTERFACE)

target_compile_definitions(
    glmmd_compile_definitions
    INTERFACE
        $<$<BOOL:${GLMMD_DO_NOT_USE_STD_EXECUTION}>:GLMMD_DO_NOT_USE_STD_EXECUTION>
)

file(GLOB GLMMD_CORE_SOURCES core/*.cpp)
add_library(glmmd_core ${GLMMD_CORE_SOURCES})
add_library(glmmd::core ALIAS glmmd_core)

find_package(Bullet REQUIRED)
find_package(glm REQUIRED)

if((NOT GLMMD_DO_NOT_USE_STD_EXECUTION) AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"
                                            ))
    find_package(TBB REQUIRED)
    set(GLMMD_TBB_LIBRARY TBB::tbb)
endif()

target_include_directories(glmmd_core PUBLIC ${BULLET_INCLUDE_DIR}
                                             "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(
    glmmd_core PUBLIC glmmd_compiler_flags glmmd_compile_definitions
                      ${GLMMD_TBB_LIBRARY} glm::glm ${BULLET_LIBRARIES})

target_compile_definitions(glmmd_core PUBLIC GLM_FORCE_RADIANS)

file(GLOB GLMMD_FILES_SOURCES files/*.cpp)
add_library(glmmd_files ${GLMMD_FILES_SOURCES})
add_library(glmmd::files ALIAS glmmd_files)

target_include_directories(glmmd_files PUBLIC "${PROJECT_SOURCE_DIR}/include")

set(GLMMD_FILES_LIBRARIES glmmd_core stb::stb)
if(GLMMD_USE_ICU)
    find_package(ICU REQUIRED COMPONENTS uc)
    list(APPEND GLMMD_FILES_LIBRARIES ICU::uc)
    target_compile_definitions(glmmd_files PUBLIC GLMMD_USE_ICU)
endif()

target_link_libraries(glmmd_files PUBLIC ${GLMMD_FILES_LIBRARIES})

if(GLMMD_BUILD_RENDERER)
    file(GLOB GLMMD_RENDER_SOURCES render/*.cpp)
    add_library(glmmd_render ${GLMMD_RENDER_SOURCES})
    add_library(glmmd::render ALIAS glmmd_render)

    target_include_directories(glmmd_render
                               PUBLIC "${PROJECT_SOURCE_DIR}/include")

    target_link_libraries(glmmd_render
                          PUBLIC glmmd_core opengl_framework::opengl_framework)
endif()

add_library(glmmd INTERFACE)
add_library(glmmd::glmmd ALIAS glmmd)
if(GLMMD_BUILD_RENDERER)
    target_link_libraries(glmmd INTERFACE glmmd_core glmmd_files glmmd_render)
else()
    target_link_libraries(glmmd INTERFACE glmmd_core glmmd_files)
endif()
