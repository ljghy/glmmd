#include <iostream>
#include <fstream>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <json.hpp>

#include <glmmd/core/SimpleAnimator.h>
#include <glmmd/files/PmxFileLoader.h>
#include <glmmd/files/VmdFileLoader.h>

#include "Context.h"

inline void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

Context::Context(const std::string &initFile)
{
    std::ifstream fin(initFile);
    m_initData = json::parse(fin);
    initWindow();
    initImGui();
    initOpenAL();

    m_chatSession = std::make_unique<ChatSession>();

    loadResources();

    m_camera.projType       = glmmd::CameraProjectionType::Perspective;
    m_camera.position       = glm::vec3(0.0f, 16.0f, -16.0f);
    m_lighting.direction    = glm::normalize(glm::vec3(-1.f, -2.f, 1.f));
    m_lighting.color        = glm::vec3(0.6f);
    m_lighting.ambientColor = glm::vec3(1.f);
}

void Context::initWindow()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    if (m_initData.find("MSAA") != m_initData.end() &&
        m_initData["MSAA"].get<int>() > 1)
        glfwWindowHint(GLFW_SAMPLES, m_initData["MSAA"].get<int>());

    m_window = glfwCreateWindow(1600, 900, "Viewer", NULL, NULL);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    if (m_window == nullptr)
    {
        throw std::runtime_error("Failed to create window.");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD.");
    }
}

void Context::initImGui()
{
    const char *glsl_version = "#version 130";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Context::initOpenAL()
{
    m_openALDevice = alcOpenDevice(nullptr);
    if (!m_openALDevice)
        throw std::runtime_error("Failed to open OpenAL device.");

    m_openALContext = alcCreateContext(m_openALDevice, nullptr);
    if (!m_openALContext)
        throw std::runtime_error("Failed to create OpenAL context.");

    if (alcMakeContextCurrent(m_openALContext) == ALC_FALSE)
        throw std::runtime_error("Failed to make OpenAL context current.");
}

void Context::loadResources()
{
    try
    {
        m_modelData = std::make_shared<glmmd::ModelData>();
        glmmd::PmxFileLoader loader(m_initData["model"], true);
        loader.load(*m_modelData);

        std::cout << "Model loaded from: " << m_initData["model"] << '\n';
        std::cout << "Name: " << m_modelData->info.modelName << '\n';
        std::cout << "Comment: " << m_modelData->info.comment << '\n';
        std::cout << std::endl;

        m_model         = std::make_unique<glmmd::Model>(m_modelData);
        m_modelRenderer = std::make_unique<glmmd::ModelRenderer>(m_modelData);

        glmmd::VmdFileLoader vmdLoader(
            m_initData["motions"]["idle_base"].get<std::string>(), *m_modelData,
            true);

        auto baseIdleMotion = std::make_shared<glmmd::FixedMotionClip>(true);
        vmdLoader.load(*baseIdleMotion);

        std::vector<std::shared_ptr<const glmmd::Motion>> idleMotions;
        for (const auto &idleMotionPath : m_initData["motions"]["idle_rand"])
        {
            glmmd::VmdFileLoader vmdLoader(idleMotionPath.get<std::string>(),
                                           *m_modelData, true);

            auto motion = std::make_shared<glmmd::FixedMotionClip>(false);
            vmdLoader.load(*motion);
            idleMotions.emplace_back(motion);
        }

        m_animators.emplace_back(std::make_unique<IdleAnimator>(
            m_modelData, baseIdleMotion, idleMotions, 10.f, 0.5f));
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Context::updateCamera(float deltaTime)
{
    auto &io = ImGui::GetIO();

    m_camera.resize(m_windowWidth, m_windowHeight);

    if (!io.WantCaptureMouse)
    {
        ImVec2          mouseDelta  = io.MouseDelta;
        constexpr float sensitivity = glm::radians(0.1f);

        if (io.MouseDown[1])
            m_camera.rotateAround(glm::vec3(0.f), -mouseDelta.x * sensitivity,
                                  -mouseDelta.y * sensitivity);

        if (io.MouseDown[2])
        {
            float vel = 5.f * glm::tan(m_camera.fovy * 0.5f);
            m_camera.position +=
                -vel * mouseDelta.x * deltaTime * m_camera.right;
            m_camera.position += vel * mouseDelta.y * deltaTime * m_camera.up;
        }

        if (m_camera.projType == glmmd::CameraProjectionType::Perspective)
        {
            m_camera.fovy -= glm::radians(5.f) * io.MouseWheel;
            m_camera.fovy = glm::clamp(m_camera.fovy, glm::radians(1.f),
                                       glm::radians(120.f));
        }
        else
        {
            m_camera.width -= 5.f * io.MouseWheel;
            m_camera.width = glm::clamp(m_camera.width, 1.f, 100.f);
        }
    }

    if (!io.WantCaptureKeyboard)
    {
        constexpr float vel = 25.f;
        if (io.KeysDown[GLFW_KEY_W])
            m_camera.position += vel * deltaTime * m_camera.front;
        if (io.KeysDown[GLFW_KEY_S])
            m_camera.position -= vel * deltaTime * m_camera.front;
        if (io.KeysDown[GLFW_KEY_A])
            m_camera.position -= vel * deltaTime * m_camera.right;
        if (io.KeysDown[GLFW_KEY_D])
            m_camera.position += vel * deltaTime * m_camera.right;
    }
}

void Context::chatControl()
{
    ImGui::Begin("Chat");

    if (m_chatSession->getState() == ChatSessionState::Idle)
    {
        static char text[256] = "";
        ImGui::InputText("##Text", text, 256);
        ImGui::SameLine();
        if (ImGui::Button("Send"))
        {
            m_chatSession->sendText(text);
            text[0] = '\0';
        }
    }
    else if (m_chatSession->getState() == ChatSessionState::Ready)
    {
        m_chatSession->setState(ChatSessionState::Playing);

        std::cout << m_chatSession->response() << std::endl;

        m_animators.emplace_back(std::make_unique<glmmd::SimpleAnimator>(
            std::make_shared<VisemeMotion>(*m_modelData,
                                           m_chatSession->visemes())));

        const auto &audioFile = m_chatSession->audioFile();
        alGenBuffers(1, &m_audioBuffer);
        alBufferData(
            m_audioBuffer, AL_FORMAT_MONO_FLOAT32, audioFile.samples[0].data(),
            static_cast<ALsizei>(audioFile.samples[0].size() * sizeof(float)),
            audioFile.getSampleRate());

        alGenSources(1, &m_audioSource);
        alSourcei(m_audioSource, AL_BUFFER, m_audioBuffer);
        alSourcef(m_audioSource, AL_SEC_OFFSET, 0.f);

        std::thread audioThread(
            [this]()
            {
                alSourcef(m_audioSource, AL_GAIN, 12.5f);
                alSourcePlay(m_audioSource);
                ALint state = AL_PLAYING;
                while (state == AL_PLAYING)
                {
                    alGetSourcei(m_audioSource, AL_SOURCE_STATE, &state);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                alDeleteSources(1, &m_audioSource);
                alDeleteBuffers(1, &m_audioBuffer);
                m_chatSession->setState(ChatSessionState::Idle);
            });
        audioThread.detach();
    }

    ImGui::End();
}

void Context::updateModel()
{
    m_animators.remove_if([](const auto &animator)
                          { return animator->isFinished(); });

    m_model->resetLocalPose();
    for (const auto &animator : m_animators)
    {
        glmmd::ModelPose pose(m_modelData);
        animator->getLocalPose(pose);
        m_model->pose() += pose;
    }
    m_model->solvePose();
}

void Context::run()
{
    for (auto &animator : m_animators)
        animator->reset();

    updateModel();

    m_physicsWorld.setupModelPhysics(*m_model, true);

    auto &io = ImGui::GetIO();

    ImVec4 clearColor = ImVec4(0.f, 0.f, 0.f, 0.f);
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                     clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);

        float deltaTime = io.DeltaTime;

        auto physicsStart = std::chrono::high_resolution_clock::now();
        m_physicsWorld.update(deltaTime, 1, 1.f / 120.f);
        auto physicsEnd = std::chrono::high_resolution_clock::now();
        auto physicsDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                physicsEnd - physicsStart)
                .count();

        auto modelUpdateStart = std::chrono::high_resolution_clock::now();

        updateModel();
        m_modelRenderer->renderData().init();
        m_model->pose().applyToRenderData(m_modelRenderer->renderData());

        auto modelUpdateEnd = std::chrono::high_resolution_clock::now();
        auto modelUpdateDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                modelUpdateEnd - modelUpdateStart)
                .count();

        auto renderStart = std::chrono::high_resolution_clock::now();
        updateCamera(deltaTime);
        m_modelRenderer->render(m_camera, m_lighting);
        auto renderEnd = std::chrono::high_resolution_clock::now();
        auto renderDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                renderEnd - renderStart)
                .count();

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Physics: %.3f ms", physicsDur * 1000.f);
        ImGui::Text("Model update: %.3f ms", modelUpdateDur * 1000.f);
        ImGui::Text("Render: %.3f ms", renderDur * 1000.f);
        ImGui::Text("Total: %.3f ms",
                    (physicsDur + modelUpdateDur + renderDur) * 1000.f);

        chatControl();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

Context::~Context()
{
    glmmd::ModelRenderer::releaseSharedToonTextures();

    alcMakeContextCurrent(m_openALContext);
    alcDestroyContext(m_openALContext);
    alcCloseDevice(m_openALDevice);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
