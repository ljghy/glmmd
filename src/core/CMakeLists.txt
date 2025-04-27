file(GLOB GLMMD_CORE_SOURCES *.cpp)
add_library(glmmd_core ${GLMMD_CORE_SOURCES})
add_library(glmmd::core ALIAS glmmd_core)

find_package(Bullet REQUIRED)

target_include_directories(glmmd_core PUBLIC ${BULLET_INCLUDE_DIR}
                                             "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(glmmd_core PUBLIC glmmd_config glm::glm
                                        ${BULLET_LIBRARIES})

if(GLMMD_USE_TBB)
    find_package(TBB REQUIRED)
    target_compile_definitions(glmmd_core PUBLIC GLMMD_USE_TBB)
    target_link_libraries(glmmd_core PUBLIC TBB::tbb)
endif()
