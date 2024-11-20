file(GLOB GLMMD_CORE_SOURCES *.cpp)
add_library(glmmd_core ${GLMMD_CORE_SOURCES})
add_library(glmmd::core ALIAS glmmd_core)

find_package(Bullet REQUIRED)

if(NOT GLMMD_DONT_PARALLELIZE)
    find_package(TBB)
    if(TBB_FOUND)
        set(GLMMD_TBB_LIBRARY TBB::tbb)
        target_compile_definitions(glmmd_core PUBLIC GLMMD_USE_TBB)
    endif()
endif()

target_include_directories(glmmd_core PUBLIC ${BULLET_INCLUDE_DIR}
                                             "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(
    glmmd_core PUBLIC glmmd_compiler_flags glmmd_compile_definitions
                      ${GLMMD_TBB_LIBRARY} glm::glm ${BULLET_LIBRARIES})

target_compile_definitions(glmmd_core PUBLIC GLM_FORCE_RADIANS)
