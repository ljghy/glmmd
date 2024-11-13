file(GLOB GLMMD_FILES_SOURCES *.cpp)
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
