#ifndef VIEWER_CONTEXT_H_
#define VIEWER_CONTEXT_H_

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <memory>

#include <glmmd/core/Model.h>
#include <glmmd/core/PhysicsWorld.h>
#include <glmmd/render/ModelRenderer.h>

#include "json.hpp"
#include "SimpleAnimator.h"

using json = nlohmann::json;

class Context
{
public:
    Context(const std::string &initFile);
    ~Context();

    void run();

private:
    void initWindow();
    void initImGui();
    void initFBO();
    void loadResources();

    void updateModelPose(size_t i);

    void updateCamera(float deltaTime);

private:
    json m_initData;

    GLFWwindow *m_window;

    int m_viewportWidth;
    int m_viewportHeight;

    int m_shadowMapWidth  = 1024;
    int m_shadowMapHeight = 1024;

    std::vector<std::shared_ptr<glmmd::ModelData>> m_modelData;

    std::vector<glmmd::Model>         m_models;
    std::vector<glmmd::ModelRenderer> m_modelRenderers;

    std::vector<std::unique_ptr<SimpleAnimator>> m_animators;

    glmmd::Camera   m_camera;
    glmmd::Lighting m_lighting;

    glmmd::PhysicsWorld m_physicsWorld;

    ogl::FrameBufferObject m_FBO;
    ogl::FrameBufferObject m_intermediateFBO;
    ogl::FrameBufferObject m_shadowMapFBO;
};

#endif