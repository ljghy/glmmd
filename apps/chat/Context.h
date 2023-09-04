#ifndef VIEWER_CONTEXT_H_
#define VIEWER_CONTEXT_H_

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>

#include <json.hpp>

#include <al.h>
#include <alc.h>

#include <glmmd/core/Model.h>
#include <glmmd/core/Animator.h>
#include <glmmd/core/PhysicsWorld.h>
#include <glmmd/render/ModelRenderer.h>

#include "ChatSession.h"

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
    void initOpenAL();
    void loadResources();

    float getCurrentTime();

    void addModel(const glmmd::ModelData &data);

    void updateCamera(float deltaTime);

private:
    json m_initData;

    GLFWwindow *m_window;
    int         m_windowWidth;
    int         m_windowHeight;

    ALCdevice  *m_openALDevice;
    ALCcontext *m_openALContext;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;

    std::vector<std::unique_ptr<glmmd::ModelData>> m_modelData;

    std::vector<glmmd::Model>         m_models;
    std::vector<glmmd::ModelRenderer> m_modelRenderers;

    glmmd::Camera   m_camera;
    glmmd::Lighting m_lighting;

    glmmd::PhysicsWorld m_physicsWorld;

    std::unique_ptr<ChatSession> m_chatSession;
};

#endif