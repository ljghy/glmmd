add_library(glmmd STATIC)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

target_sources(glmmd
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/PhysicsWorld.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ModelPose.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ModelPoseSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/FixedMotionClip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/RenderData.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/files/CodeConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/files/PmxFileLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/files/VmdFileLoader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/render/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/Lighting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/ModelRenderer.cpp
    
)

find_package(Bullet REQUIRED)

if (GLMMD_USE_SYSTEM_GLM)
    if (GLMMD_GLM_ROOT)
        find_package(glm
                    PATHS "${GLMMD_GLM_ROOT}" "${GLMMD_GLM_ROOT}/cmake")
    else()
        find_package(glm)
    endif()
endif()

if (NOT GLMMD_USE_SYSTEM_GLM OR NOT glm_FOUND)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

set(GLMMD_COMPILE_DEFINITIONS "")

if (GLMMD_DO_NOT_USE_STD_EXECUTION)
    list(APPEND GLMMD_COMPILE_DEFINITIONS GLMMD_DO_NOT_USE_STD_EXECUTION)
endif()

if (GLMMD_DO_NOT_FORCE_UTF8)
    list(APPEND GLMMD_COMPILE_DEFINITIONS GLMMD_DO_NOT_FORCE_UTF8)
endif()

target_compile_definitions(glmmd
PUBLIC
    ${GLMMD_COMPILE_DEFINITIONS}
)

target_include_directories(glmmd
PUBLIC
    ${BULLET_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(glmmd 
PUBLIC
    glm::glm
    glad
    ${BULLET_LIBRARIES}
    opengl_framework
    stb_image
)
