#include <iostream>
#include <fstream>
#include <algorithm>

#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
#include <numeric>
#include <execution>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>
#include <glmmd/files/VmdFileLoader.h>

#include "Context.h"

inline void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

Context::Context(const std::string &initFile)
{
    m_initData = parseJsonFile(initFile);

    initWindow();
    initImGui();
    initFBO();
    loadResources();

    m_cameraTarget          = glm::vec3(0.f);
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsClassic();

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Context::initFBO()
{
    m_viewportWidth  = 1600;
    m_viewportHeight = 900;
    int samples      = m_initData.get<int>("MSAA", 1);

    m_FBO.create();
    ogl::Texture2DCreateInfo texInfo;
    texInfo.width       = m_viewportWidth;
    texInfo.height      = m_viewportHeight;
    texInfo.samples     = samples;
    texInfo.internalFmt = GL_RGBA;
    texInfo.dataFmt     = GL_RGBA;
    m_FBO.attachColorTexture(std::make_unique<ogl::Texture2D>(texInfo));

    ogl::RenderBufferObjectCreateInfo rboInfo;
    rboInfo.width       = m_viewportWidth;
    rboInfo.height      = m_viewportHeight;
    rboInfo.samples     = samples;
    rboInfo.internalFmt = GL_DEPTH24_STENCIL8;
    m_FBO.attachDepthRenderBuffer(
        std::make_unique<ogl::RenderBufferObject>(rboInfo));

    if (!m_FBO.isComplete())
        throw std::runtime_error("Failed to create FBO.");

    m_intermediateFBO.create();
    texInfo.samples = 1;
    m_intermediateFBO.attachColorTexture(
        std::make_unique<ogl::Texture2D>(texInfo));

    if (!m_intermediateFBO.isComplete())
        throw std::runtime_error("Failed to create intermediate FBO.");

    m_shadowMapFBO.create();
    ogl::Texture2DCreateInfo shadowMapTexInfo;
    m_shadowMapWidth             = m_initData.get<int>("ShadowMapWidth", 1024);
    m_shadowMapHeight            = m_initData.get<int>("ShadowMapHeight", 1024);
    shadowMapTexInfo.width       = m_shadowMapWidth;
    shadowMapTexInfo.height      = m_shadowMapHeight;
    shadowMapTexInfo.internalFmt = GL_DEPTH_COMPONENT;
    shadowMapTexInfo.dataFmt     = GL_DEPTH_COMPONENT;
    shadowMapTexInfo.dataType    = GL_FLOAT;
    shadowMapTexInfo.wrapModeS   = GL_CLAMP_TO_BORDER;
    shadowMapTexInfo.wrapModeT   = GL_CLAMP_TO_BORDER;
    std::unique_ptr<ogl::Texture2D> shadowMapTex =
        std::make_unique<ogl::Texture2D>(shadowMapTexInfo);
    shadowMapTex->bind();
    float borderColor[] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    m_shadowMapFBO.attachDepthTexture(std::move(shadowMapTex));

    if (!m_shadowMapFBO.isComplete())
        throw std::runtime_error("Failed to create shadow map FBO.");
}

void Context::loadResources()
{
    for (const auto &modelNode : m_initData["models"].arr())
    {
        try
        {
            auto filename = modelNode.get<std::filesystem::path>("filename");
            m_modelData.push_back(glmmd::loadPmxFile(filename));
            auto modelData = m_modelData.back();
            m_models.emplace_back(modelData);
            m_modelRenderers.emplace_back(modelData);

            std::cout << "Model loaded from: " << filename.u8string() << '\n';
            std::cout << "Name: " << modelData->info.modelName << '\n';
            std::cout << "Comment: " << modelData->info.comment << '\n';
            std::cout << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    std::vector<std::vector<std::shared_ptr<glmmd::Motion>>> motions(
        m_models.size());

    for (const auto &motionNode : m_initData["motions"].arr())
    {
        try
        {
            auto modelIndex = motionNode.get<size_t>("model");
            if (modelIndex >= m_models.size())
                throw std::runtime_error("Invalid model index.");

            auto filename = motionNode.get<std::filesystem::path>("filename");
            auto vmdData  = glmmd::loadVmdFile(filename);

            bool loop = motionNode.get<bool>("loop", false);
            auto clip = std::make_shared<glmmd::FixedMotionClip>(
                vmdData->toFixedMotionClip(m_models[modelIndex].data(), loop));

            std::cout << "Motion data loaded from: " << filename.u8string()
                      << '\n';
            std::cout << "Created on: "
                      << glmmd::codeCvt<glmmd::ShiftJIS, glmmd::UTF8>(
                             vmdData->modelName)
                      << '\n';
            std::cout << "Length: " << clip->duration() << " s\n";
            std::cout << std::endl;

            motions[modelIndex].emplace_back(clip);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    for (const auto &m : motions)
        m_animators.emplace_back(std::make_unique<SimpleAnimator>(m));
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
            m_camera.rotateAround(m_cameraTarget, -mouseDelta.x * sensitivity,
                                  -mouseDelta.y * sensitivity);

        if (io.MouseDown[2])
        {
            float     vel = 5.f * glm::tan(m_camera.fovy * 0.5f);
            glm::vec3 translation =
                -vel * mouseDelta.x * deltaTime * m_camera.right +
                vel * mouseDelta.y * deltaTime * m_camera.up;
            m_camera.position += translation;
            m_cameraTarget += translation;
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

        glm::vec3 translation(0.f);
        if (io.KeysDown[GLFW_KEY_W])
            translation += vel * deltaTime * m_camera.front;
        if (io.KeysDown[GLFW_KEY_S])
            translation -= vel * deltaTime * m_camera.front;
        if (io.KeysDown[GLFW_KEY_A])
            translation -= vel * deltaTime * m_camera.right;
        if (io.KeysDown[GLFW_KEY_D])
            translation += vel * deltaTime * m_camera.right;

        m_camera.position += translation;
        m_cameraTarget += translation;
    }
}

void Context::saveScreenshot()
{
    int channels = 3;

    std::vector<float> screenshotBuffer(channels * m_viewportHeight *
                                        m_viewportWidth);

    m_intermediateFBO.readColorAttachment(screenshotBuffer.data(), GL_RGB,
                                          GL_FLOAT);

    std::vector<uint8_t> m_screenshotBuffer(channels * m_viewportHeight *
                                            m_viewportWidth);

    for (int i = 0; i < m_viewportHeight * m_viewportWidth * channels; ++i)
    {
        m_screenshotBuffer[i] =
            static_cast<uint8_t>(glm::round(screenshotBuffer[i] * 255.f));
    }

    for (int row = 0; row != m_viewportHeight / 2; ++row)
    {
        std::swap_ranges(
            m_screenshotBuffer.begin() + row * m_viewportWidth * channels,
            m_screenshotBuffer.begin() + (row + 1) * m_viewportWidth * channels,
            m_screenshotBuffer.begin() +
                (m_viewportHeight - row - 1) * m_viewportWidth * channels);
    }

    static int  screenshotIndex = 0;
    std::string filename =
        "screenshot_" + std::to_string(screenshotIndex++) + ".png";
    stbi_write_png(filename.c_str(), m_viewportWidth, m_viewportHeight,
                   channels, m_screenshotBuffer.data(),
                   m_viewportWidth * channels);
}

void Context::updateModelPose(size_t i)
{
    auto &model = m_models[i];
    model.resetLocalPose();
    m_animators[i]->getLocalPose(model.pose());
    model.solvePose();
}

void Context::run()
{
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_animators[i]->reset();
        updateModelPose(i);
        m_physicsWorld.setupModelPhysics(m_models[i], true);
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
        m_physicsWorld.update(deltaTime, 10, 1.f / 60.f);
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

        for (const auto &renderer : m_modelRenderers)
            renderer.fillBuffers();

        // Render shadow map

        static bool renderShadow = true;

        if (renderShadow)
        {
            m_shadowMapFBO.bind();
            glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
            glClear(GL_DEPTH_BUFFER_BIT);
            for (const auto &renderer : m_modelRenderers)
                renderer.renderShadowMap(m_lighting);
            m_shadowMapFBO.unbind();
        }

        // Render models

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

        m_FBO.bind();

        int currentViewportWidth =
            static_cast<int>(std::round(ImGui::GetWindowWidth()));
        int currentViewportHeight =
            static_cast<int>(std::round(ImGui::GetWindowHeight()));
        if (currentViewportWidth > 0 && currentViewportHeight > 0 &&
            (m_viewportWidth != currentViewportWidth ||
             m_viewportHeight != currentViewportHeight))
        {
            m_viewportWidth  = currentViewportWidth;
            m_viewportHeight = currentViewportHeight;
            m_FBO.resize(m_viewportWidth, m_viewportHeight);
            m_intermediateFBO.resize(m_viewportWidth, m_viewportHeight);
        }

        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                     clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, m_viewportWidth, m_viewportHeight);

        updateCamera(deltaTime);
        for (const auto &renderer : m_modelRenderers)
            renderer.render(m_camera, m_lighting,
                            renderShadow
                                ? m_shadowMapFBO.depthTextureAttachment()
                                : nullptr);
        m_FBO.unbind();

        m_FBO.bindRead();
        m_intermediateFBO.bindDraw();
        glBlitFramebuffer(0, 0, m_viewportWidth, m_viewportHeight, 0, 0,
                          m_viewportWidth, m_viewportHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        m_intermediateFBO.unbind();

        ImGui::Image(
            (void *)(uintptr_t)m_intermediateFBO.colorTextureAttachment()->id(),
            ImVec2(static_cast<float>(m_viewportWidth),
                   static_cast<float>(m_viewportHeight)),
            ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
        ImGui::PopStyleVar();

        auto renderEnd = std::chrono::high_resolution_clock::now();
        auto renderDur =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                renderEnd - renderStart)
                .count();

        if (io.WantCaptureKeyboard)
            if (ImGui::IsKeyPressed(ImGuiKey_F2))
                saveScreenshot();

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Physics: %.3f ms", physicsDur * 1000.f);
        ImGui::Text("Model update: %.3f ms", modelUpdateDur * 1000.f);
        ImGui::Text("Render: %.3f ms", renderDur * 1000.f);
        ImGui::Text("Total: %.3f ms",
                    (physicsDur + modelUpdateDur + renderDur) * 1000.f);

        static bool pause = false;
        if (ImGui::Button(pause ? "Resume" : "Pause"))
        {
            if (pause)
                for (auto &animator : m_animators)
                    animator->resume();
            else
                for (auto &animator : m_animators)
                    animator->pause();
            pause = !pause;
        }

        static bool physics = true;
        if (ImGui::Checkbox("Physics", &physics))
        {
            if (physics)
            {
                for (auto &model : m_models)
                    m_physicsWorld.setupModelPhysics(model, true);
            }
            else
            {
                for (auto &model : m_models)
                    m_physicsWorld.clearModelPhysics(model);
            }
        }

        if (ImGui::Button("Reset"))
        {
            for (auto &animator : m_animators)
                animator->reset();
        }

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

        ImGui::Checkbox("Render shadow", &renderShadow);
        if (ImGui::SliderFloat3("Light direction", &m_lighting.direction.x,
                                -1.f, 1.f))
            m_lighting.direction = glm::normalize(m_lighting.direction);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(m_window);
    }
}

Context::~Context()
{
    glmmd::ModelRenderer::releaseSharedToonTextures();

    m_modelRenderers.clear();
    m_FBO.destroy();
    m_intermediateFBO.destroy();
    m_shadowMapFBO.destroy();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
