add_library(glmmd_compiler_flags INTERFACE)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(
    glmmd_compiler_flags
    INTERFACE
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wshadow;-Wformat=2;-Wunused>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3;/MP;/wd4996;/Zc:__cplusplus;/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>>"
)

target_compile_features(glmmd_compiler_flags INTERFACE cxx_std_17)

add_library(glmmd_compile_definitions INTERFACE)

target_compile_definitions(
    glmmd_compile_definitions
    INTERFACE $<$<BOOL:${GLMMD_DONT_PARALLELIZE}>:GLMMD_DONT_PARALLELIZE>)

add_subdirectory(core)
add_subdirectory(files)

add_library(glmmd INTERFACE)
add_library(glmmd::glmmd ALIAS glmmd)
target_link_libraries(glmmd INTERFACE glmmd::core glmmd::files)
