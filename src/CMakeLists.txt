add_library(glmmd_config INTERFACE)

target_compile_features(glmmd_config INTERFACE cxx_std_20)

set(glmmd_compile_definitions GLM_FORCE_RADIANS GLM_ENABLE_EXPERIMENTAL)

if(GLMMD_DONT_PARALLELIZE)
    list(APPEND glmmd_compile_definitions GLMMD_DONT_PARALLELIZE)
else()
    include(CheckCXXSymbolExists)

    check_cxx_symbol_exists(_LIBCPP_VERSION "version" GLMMD_IS_LIBCXX)
    check_cxx_symbol_exists(__GLIBCXX__ "version" GLMMD_IS_LIBSTDCXX)

    if(GLMMD_IS_LIBCXX)
        target_compile_options(glmmd_config INTERFACE -fexperimental-library)
    elseif(GLMMD_IS_LIBSTDCXX)
        set(GLMMD_USE_TBB TRUE)
    endif()
endif()

target_compile_definitions(glmmd_config INTERFACE ${glmmd_compile_definitions})

add_subdirectory(core)
add_subdirectory(files)

add_library(glmmd INTERFACE)
add_library(glmmd::glmmd ALIAS glmmd)
target_link_libraries(glmmd INTERFACE glmmd::core glmmd::files)
