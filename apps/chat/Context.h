#ifndef CHAT_CONTEXT_H_
#define CHAT_CONTEXT_H_

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <memory>
#include <list>

#include <json.hpp>

#include <al.h>
#include <alc.h>
#include <alext.h>

#include <glmmd/core/Model.h>
#include <glmmd/core/PhysicsWorld.h>
#include <glmmd/core/Animator.h>
#include <glmmd/render/ModelRenderer.h>

#include "ChatSession.h"
#include "IdleAnimator.h"

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

    void chatControl();

    void updateModel();
    void updateCamera(float deltaTime);

private:
    json m_initData;

    GLFWwindow *m_window;
    int         m_windowWidth;
    int         m_windowHeight;

    ALCdevice  *m_openALDevice;
    ALCcontext *m_openALContext;

    ALuint m_audioBuffer;
    ALuint m_audioSource;

    std::shared_ptr<glmmd::ModelData>     m_modelData;
    std::unique_ptr<glmmd::Model>         m_model;
    std::unique_ptr<glmmd::ModelRenderer> m_modelRenderer;

    std::list<std::unique_ptr<glmmd::Animator>> m_animators;

    glmmd::Camera   m_camera;
    glmmd::Lighting m_lighting;

    glmmd::PhysicsWorld m_physicsWorld;

    std::unique_ptr<ChatSession> m_chatSession;

};

#endif