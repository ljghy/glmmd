file(GLOB GLMMD_CORE_SOURCES *.cpp)
add_library(glmmd_core ${GLMMD_CORE_SOURCES})
add_library(glmmd::core ALIAS glmmd_core)

target_include_directories(glmmd_core PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(glmmd_core PUBLIC glmmd_config glm::glm)

if(GLMMD_USE_TBB)
    find_package(TBB REQUIRED)
    target_compile_definitions(glmmd_core PUBLIC GLMMD_USE_TBB)
    target_link_libraries(glmmd_core PUBLIC TBB::tbb)
endif()

if(GLMMD_USE_BULLET)
    find_package(Bullet REQUIRED)
    target_include_directories(glmmd_core PUBLIC ${BULLET_INCLUDE_DIRS})
    target_link_libraries(glmmd_core PUBLIC ${BULLET_LIBRARIES})
else()
    target_compile_definitions(glmmd_core PUBLIC GLMMD_DONT_USE_BULLET)
endif()
