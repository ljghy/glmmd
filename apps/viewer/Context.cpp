#include <iostream>
#include <fstream>

#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
#include <algorithm>
#include <numeric>
#include <execution>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <json.hpp>

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
    initFBO();
    loadResources();

    m_camera.projType       = glmmd::CameraProjectionType::Perspective;
    m_camera.position       = glm::vec3(0.0f, 14.0f, -24.0f);
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

void Context::initFBO()
{
    m_viewportWidth  = 1600;
    m_viewportHeight = 900;
    int samples      = m_initData.find("MSAA") != m_initData.end()
                           ? m_initData["MSAA"].get<int>()
                           : 1;

    m_FBO.create();
    Texture2DCreateInfo texInfo;
    texInfo.width       = m_viewportWidth;
    texInfo.height      = m_viewportHeight;
    texInfo.samples     = samples;
    texInfo.internalFmt = GL_RGB;
    texInfo.dataFmt     = GL_RGB;
    m_FBO.attachColorTexture(std::make_unique<Texture2D>(texInfo));

    RenderBufferObjectCreateInfo rboInfo;
    rboInfo.width       = m_viewportWidth;
    rboInfo.height      = m_viewportHeight;
    rboInfo.samples     = samples;
    rboInfo.internalFmt = GL_DEPTH24_STENCIL8;
    m_FBO.attachDepthRenderBuffer(
        std::make_unique<RenderBufferObject>(rboInfo));

    if (!m_FBO.isComplete())
        throw std::runtime_error("Failed to create FBO.");

    m_intermediateFBO.create();
    texInfo.samples = 1;
    m_intermediateFBO.attachColorTexture(std::make_unique<Texture2D>(texInfo));
}

void Context::loadResources()
{
    for (const auto &modelNode : m_initData["models"])
    {
        auto &modelData =
            m_modelData.emplace_back(std::make_shared<glmmd::ModelData>());

        try
        {
            auto filename = modelNode["filename"].get<std::string>();
            glmmd::PmxFileLoader loader(filename, true);
            loader.load(*modelData);

            std::cout << "Model loaded from: " << filename << '\n';
            std::cout << "Name: " << modelData->info.modelName << '\n';
            std::cout << "Comment: " << modelData->info.comment << '\n';
            std::cout << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }

        m_models.emplace_back(modelData);
        m_modelRenderers.emplace_back(modelData);
        m_animators.emplace_back();
    }

    for (const auto &motionNode : m_initData["motions"])
    {
        try
        {
            bool loop = false;
            if (motionNode.find("loop") != motionNode.end())
                loop = motionNode["loop"].get<bool>();

            auto clip       = std::make_shared<glmmd::FixedMotionClip>(loop);
            auto modelIndex = motionNode["model"].get<size_t>();
            auto filename   = motionNode["filename"].get<std::string>();
            glmmd::VmdFileLoader loader(filename, *m_modelData[modelIndex],
                                        true);
            loader.load(*clip);

            std::cout << "Motion data loaded from: " << filename << '\n';
            std::cout << "Created for: " << loader.modelName() << '\n';
            std::cout << std::endl;

            auto &animator = m_animators[modelIndex].emplace_back(clip);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}

void Context::updateCamera(float deltaTime)
{
    auto &io = ImGui::GetIO();

    m_camera.resize(m_viewportWidth, m_viewportHeight);

    if (io.WantCaptureMouse)
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

    if (io.WantCaptureKeyboard)
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

void Context::updateModelPose(size_t i)
{
    auto &model = m_models[i];
    model.resetLocalPose();
    for (const auto &animator : m_animators[i])
    {
        glmmd::ModelPose pose(m_modelData[i]);
        animator.getLocalPose(pose);
        model.pose() += pose;
    }
    model.solvePose();
}

void Context::run()
{
    size_t modelIndex = 0;
    for (auto &model : m_models)
    {
        for (auto &animator : m_animators[modelIndex])
            animator.reset();
        updateModelPose(modelIndex);

        m_physicsWorld.setupModelPhysics(model, true);
        ++modelIndex;
    }

    auto &io = ImGui::GetIO();

    ImVec4 clearColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float deltaTime = io.DeltaTime;

        auto physicsStart = std::chrono::high_resolution_clock::now();
        m_physicsWorld.update(deltaTime, 1, 1.f / 120.f);
        auto physicsEnd = std::chrono::high_resolution_clock::now();
        auto physicsDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                physicsEnd - physicsStart)
                .count();

        auto modelUpdateStart = std::chrono::high_resolution_clock::now();

#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
        std::vector<size_t> modelIndices(m_models.size());
        std::iota(modelIndices.begin(), modelIndices.end(), 0);
        std::for_each(std::execution::par, modelIndices.begin(),
                      modelIndices.end(),
                      [&](size_t i)
#else
        for (size_t i = 0; i < m_models.size(); ++i)
#endif
                      {
                          updateModelPose(i);
                          m_modelRenderers[i].renderData().init();
                          m_models[i].pose().applyToRenderData(
                              m_modelRenderers[i].renderData());
                      }
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
        );
#endif

        auto modelUpdateEnd = std::chrono::high_resolution_clock::now();
        auto modelUpdateDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                modelUpdateEnd - modelUpdateStart)
                .count();

        auto renderStart = std::chrono::high_resolution_clock::now();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

        m_FBO.bind();

        if (m_viewportWidth != ImGui::GetWindowWidth() ||
            m_viewportHeight != ImGui::GetWindowHeight())
        {
            m_viewportWidth  = ImGui::GetWindowWidth();
            m_viewportHeight = ImGui::GetWindowHeight();
            m_FBO.resize(m_viewportWidth, m_viewportHeight);
            m_intermediateFBO.resize(m_viewportWidth, m_viewportHeight);
        }

        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                     clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, m_viewportWidth, m_viewportHeight);

        updateCamera(deltaTime);
        for (auto &renderer : m_modelRenderers)
            renderer.render(m_camera, m_lighting);
        m_FBO.unbind();

        m_FBO.bindRead();
        m_intermediateFBO.bindDraw();
        glBlitFramebuffer(0, 0, m_viewportWidth, m_viewportHeight, 0, 0,
                          m_viewportWidth, m_viewportHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        m_intermediateFBO.unbind();

        ImGui::Image(
            (void *)(uintptr_t)m_intermediateFBO.colorTextureAttachment()->id(),
            ImVec2(m_viewportWidth, m_viewportHeight), ImVec2(0, 1),
            ImVec2(1, 0));
        ImGui::End();
        ImGui::PopStyleVar();

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

        static glm::vec3 gravity = glm::vec3(0.f, -9.8f, 0.f);
        if (ImGui::SliderFloat3("Gravity", &gravity.x, -10.f, 10.f))
            m_physicsWorld.setGravity(gravity);
        ImGui::ColorEdit4("Clear Color", &clearColor.x);
        static bool ortho =
            m_camera.projType == glmmd::CameraProjectionType::Orthographic;
        if (ImGui::Checkbox("Ortho", &ortho))
            m_camera.projType = ortho
                                    ? glmmd::CameraProjectionType::Orthographic
                                    : glmmd::CameraProjectionType::Perspective;

        static bool renderEdge = true;
        if (ImGui::Checkbox("Render edge", &renderEdge))
        {
            for (auto &renderer : m_modelRenderers)
                if (renderEdge)
                    renderer.renderFlag() |= glmmd::MODEL_RENDER_FLAG_EDGE;
                else
                    renderer.renderFlag() &= ~glmmd::MODEL_RENDER_FLAG_EDGE;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

Context::~Context()
{
    glmmd::ModelRenderer::releaseSharedToonTextures();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
