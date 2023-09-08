project(glmmd_chat)

add_executable(chat)

target_sources(chat
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VisemeMotion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IdleAnimator.cpp
)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(OpenAL REQUIRED)

target_link_libraries(chat
PRIVATE
    glmmd
    glfw
    imgui
    nlohmann_json
    Python3::Python
    OpenAL::OpenAL
)

add_custom_command(TARGET chat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets)
