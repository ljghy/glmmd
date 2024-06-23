#ifndef VIEWER_CONTEXT_H_
#define VIEWER_CONTEXT_H_

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <memory>

#include <glmmd/core/Model.h>
#include <glmmd/core/PhysicsWorld.h>
#include <glmmd/render/ModelRenderer.h>

#include "JsonParser.hpp"
#include "SimpleAnimator.h"
#include "AxesRenderer.h"
#include "GridRenderer.h"

class Context
{
    friend void dropCallback(GLFWwindow *window, int count, const char **paths);

public:
    Context(const std::string &initFile);
    ~Context();

    void run();

private:
    void initWindow();
    void initImGui();
    void initFBO();
    void initRenderers();
    void loadResources();

    bool loadModel(const std::filesystem::path &path);
    void removeModel(size_t i);

    void loadMotion(const std::filesystem::path &path, size_t modelIndex,
                    const JsonNode &config);

    void updateModelPose(size_t i);

    void updateCamera(float deltaTime);

    void saveScreenshot();

private:
    JsonNode m_initData;

    GLFWwindow *m_window;

    int m_viewportWidth;
    int m_viewportHeight;

    int m_shadowMapWidth  = 1024;
    int m_shadowMapHeight = 1024;

    std::vector<std::shared_ptr<glmmd::ModelData>> m_modelData;
    std::vector<size_t>                            m_modelIndexMap;

    std::vector<glmmd::Model>         m_models;
    std::vector<glmmd::ModelRenderer> m_modelRenderers;

    std::vector<std::unique_ptr<SimpleAnimator>> m_animators;

    int m_selectedModelIndex = -1;

    std::unique_ptr<AxesRenderer> m_axesRenderer;
    std::unique_ptr<GridRenderer> m_gridRenderer;

    glm::vec3       m_cameraTarget;
    glmmd::Camera   m_camera;
    glmmd::Lighting m_lighting;

    glmmd::PhysicsWorld m_physicsWorld;

    ogl::FrameBufferObject m_FBO;
    ogl::FrameBufferObject m_intermediateFBO;
    ogl::FrameBufferObject m_shadowMapFBO;
};

#endif