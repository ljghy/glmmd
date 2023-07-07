add_executable(viewer)

target_sources(viewer
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_include_directories(viewer
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/nlohmann_json/include
)

target_link_libraries(viewer
PRIVATE
    glmmd
    glfw
    imgui
)
