add_library(glmmd STATIC)

target_sources(glmmd
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/PhysicsWorld.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ModelPose.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ModelPoseSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/FixedMotionClip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Animator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/RenderData.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/files/CodeConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/files/PmxFileLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/files/VmdFileLoader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/render/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/ModelRenderer.cpp
    
)

find_package(Bullet REQUIRED)

if (GLMMD_DO_NOT_USE_STD_EXECUTION)
    target_compile_definitions(glmmd
    PUBLIC
        GLMMD_DO_NOT_USE_STD_EXECUTION
    )
endif()

if (GLMMD_DO_NOT_FORCE_UTF8)
    target_compile_definitions(glmmd
    PUBLIC
        GLMMD_DO_NOT_FORCE_UTF8
    )
endif()

target_include_directories(glmmd
PUBLIC
    ${BULLET_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/libs/stb/include
    ${PROJECT_SOURCE_DIR}/libs/glad/include
    ${PROJECT_SOURCE_DIR}/libs/opengl_framework/include
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(glmmd 
PUBLIC
    glm::glm
    glad
    glfw
    ${BULLET_LIBRARIES}
    opengl_framework
)
