#include <iostream>
#include <algorithm>

#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
#include <numeric>
#include <execution>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>
#include <glmmd/files/VmdFileLoader.h>

#include "Context.h"

void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

void dropCallback(GLFWwindow *window, int count, const char **paths)
{
    Context *context = (Context *)glfwGetWindowUserPointer(window);
    for (int i = 0; i < count; ++i)
    {
        std::filesystem::path path =
#if __cplusplus >= 202002L
            std::u8string(paths[i], paths[i] + strlen(paths[i]));
#else
            std::filesystem::u8path(paths[i]);
#endif
        if (path.extension() == ".pmx")
        {
            if (context->loadModel(path))
                context->m_selectedModelIndex =
                    static_cast<int>(context->m_modelData.size()) - 1;
        }
        else if (path.extension() == ".vmd")
            context->loadMotion(path, context->m_selectedModelIndex,
                                JsonObj_t{});
    }
}

Context::Context(const std::string &initFile)
{

    if (std::filesystem::exists(initFile))
        m_initData = parseJsonFile(initFile);
    else
        m_initData = JsonNode{{"MSAA"_key, 4}};

    initWindow();
    initImGui();
    initFBO();
    initRenderers();
    loadResources();

    initState();

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    int initWidth  = m_initData.get<int>("WindowWidth", 1600);
    int initHeight = m_initData.get<int>("WindowHeight", 900);

    m_window = glfwCreateWindow(initWidth, initHeight, "Viewer", NULL, NULL);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetDropCallback(m_window, dropCallback);
    glfwSetWindowUserPointer(m_window, this);
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

    if (m_initData.contains("font"))
    {
        auto font = io.Fonts->AddFontFromFileTTF(
            m_initData["font"]["path"].c_str(),
            m_initData["font"].get<float>("size", 14.f), nullptr,
            io.Fonts->GetGlyphRangesJapanese());
        if (font == nullptr)
            std::cerr << "Failed to load font.\n";
    }
}

void Context::initFBO()
{
    m_viewportWidth  = 1600;
    m_viewportHeight = 900;
    int samples      = m_initData.get<int>("MSAA", 4);

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

void Context::initRenderers()
{
    m_gridRenderer = std::make_unique<InfiniteGridRenderer>();
}

bool Context::loadModel(const std::filesystem::path &path)
{
    m_modelIndexMap.push_back(m_modelData.size());
    std::shared_ptr<glmmd::ModelData> modelData(nullptr);
    std::vector<ogl::Texture2D>       gpuTextures;
    try
    {
        modelData = glmmd::loadPmxFile(
            path,
            [&](glmmd::Texture &tex)
            {
                auto &gpuTex = gpuTextures.emplace_back();
                if (!tex.data)
                {
                    std::cout << "Failed to load texture: " << tex.path << '\n';
                    return;
                }
                ogl::Texture2DCreateInfo info;
                info.width         = tex.width;
                info.height        = tex.height;
                info.data          = tex.data.get();
                info.genMipmaps    = true;
                info.internalFmt   = GL_SRGB_ALPHA;
                info.dataFmt       = GL_RGBA;
                info.dataType      = GL_UNSIGNED_BYTE;
                info.wrapModeS     = GL_CLAMP_TO_EDGE;
                info.wrapModeT     = GL_CLAMP_TO_EDGE;
                info.minFilterMode = GL_LINEAR_MIPMAP_LINEAR;
                info.magFilterMode = GL_LINEAR;
                gpuTex.create(info);
                tex.data.reset();
            });
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    if (!modelData)
        return false;

    std::cout << "Model loaded from: " << path.u8string() << '\n';
    std::cout << "Name: " << modelData->info.modelName << '\n';
    std::cout << "Comment: " << modelData->info.comment << '\n';
    std::cout << std::endl;

    m_modelData.push_back(modelData);
    auto &renderer = m_modelRenderers.emplace_back(modelData, false);
    for (size_t i = 0; i < gpuTextures.size(); ++i)
        renderer.setTexture(i, std::move(gpuTextures[i]));
    m_models.emplace_back(modelData);
    m_animators.emplace_back(std::make_unique<SimpleAnimator>(modelData));
    return true;
}

void Context::removeModel(size_t i)
{
    m_physicsWorld.clearModelPhysics(m_models[i]);
    m_animators.erase(m_animators.begin() + i);
    m_modelRenderers.erase(m_modelRenderers.begin() + i);
    m_models.erase(m_models.begin() + i);
    m_modelData.erase(m_modelData.begin() + m_modelIndexMap[i]);
    for (size_t j = i + 1; j < m_modelIndexMap.size(); ++j)
        --m_modelIndexMap[j];
    m_modelIndexMap.erase(m_modelIndexMap.begin() + i);
}

void Context::loadMotion(const std::filesystem::path &path, size_t modelIndex,
                         const JsonNode &config)
{
    if (modelIndex >= m_modelIndexMap.size())
    {
        std::cerr << "Invalid model index.\n";
        return;
    }
    modelIndex = m_modelIndexMap[modelIndex];
    std::shared_ptr<glmmd::VmdData> vmdData(nullptr);
    try
    {
        vmdData = glmmd::loadVmdFile(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    if (!vmdData)
        return;

    bool loop = config.get<bool>("loop", false);
    auto clip = std::make_shared<glmmd::FixedMotionClip>(
        vmdData->toFixedMotionClip(m_models[modelIndex].data(), loop));

    m_animators[modelIndex]->addMotion(clip);

    std::cout << "Motion data loaded from: " << path.u8string() << '\n';
    std::cout << "Created on: "
              << glmmd::codeCvt<glmmd::ShiftJIS, glmmd::UTF8>(
                     vmdData->modelName)
              << '\n';
    std::cout << "Duration: " << clip->duration() << " s\n";
    std::cout << std::endl;
}

void Context::loadResources()
{
    if (m_initData.contains("models"))
        for (const auto &modelNode : m_initData["models"].arr())
            loadModel(modelNode.get<std::filesystem::path>("filename"));

    if (m_initData.contains("motions"))
        for (const auto &motionNode : m_initData["motions"].arr())
            loadMotion(motionNode.get<std::filesystem::path>("filename"),
                       motionNode.get<size_t>("model"), motionNode);
}

void Context::handleInput(float deltaTime)
{
    auto &io = ImGui::GetIO();

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
        m_camera.fovy =
            glm::clamp(m_camera.fovy, glm::radians(1.f), glm::radians(120.f));
    }
    else
    {
        m_camera.width -= 5.f * io.MouseWheel;
        m_camera.width = glm::clamp(m_camera.width, 1.f, 100.f);
    }
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

void Context::updateModelPose(size_t i)
{
    auto &model = m_models[i];
    model.resetLocalPose();
    m_animators[i]->getLocalPose(model.pose());
    model.solvePose();
}

void Context::initState()
{
    m_state.paused = true;

    m_state.physicsEnabled      = false;
    m_state.physicsFPSSelection = 1;

    m_state.gravity = glm::vec3(0.f, -9.8f, 0.f);

    m_state.clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);

    m_state.ortho =
        m_camera.projType == glmmd::CameraProjectionType::Orthographic;
    m_state.renderEdge         = true;
    m_state.renderShadow       = true;
    m_state.renderGroundShadow = true;
    m_state.renderAxes         = true;
    m_state.renderGrid         = true;

    m_state.shouldUpdateModels = true;
}

void Context::dockspace()
{
    ImGuiViewport *mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->WorkPos);
    ImGui::SetNextWindowSize(mainViewport->WorkSize);
    ImGui::SetNextWindowViewport(mainViewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::Begin("Main", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoBackground);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(m_window, true);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiID dockspaceId = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    static bool firstLoop = true;
    if (firstLoop)
    {
        firstLoop = false;
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

        ImGuiID controlDockId = ImGui::DockBuilderSplitNode(
            dockspaceId, ImGuiDir_Left, 0.2f, nullptr, &dockspaceId);
        ImGuiID viewportDockId = dockspaceId;

        ImGui::DockBuilderDockWindow("Control", controlDockId);
        ImGui::DockBuilderDockWindow("Viewport", viewportDockId);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void Context::updateModels()
{
#ifndef GLMMD_DO_NOT_USE_STD_EXECUTION
    std::vector<size_t> modelIndices(m_models.size());
    std::iota(modelIndices.begin(), modelIndices.end(), 0);
    std::for_each(std::execution::par, modelIndices.begin(), modelIndices.end(),
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
}

void Context::updateViewportSize()
{
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

        m_camera.resize(m_viewportWidth, m_viewportHeight);
    }
}

void Context::render()
{
    if (m_state.shouldUpdateModels)
        for (const auto &renderer : m_modelRenderers)
            renderer.fillBuffers();

    // Render shadow map

    if (m_state.renderShadow)
    {
        m_shadowMapFBO.bind();
        glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (const auto &renderer : m_modelRenderers)
            renderer.renderShadowMap(m_lighting);
        m_shadowMapFBO.unbind();
    }

    // Render models

    m_FBO.bind();

    glClearColor(m_state.clearColor.x * m_state.clearColor.w,
                 m_state.clearColor.y * m_state.clearColor.w,
                 m_state.clearColor.z * m_state.clearColor.w,
                 m_state.clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);

    for (const auto &renderer : m_modelRenderers)
        renderer.render(m_camera, m_lighting,
                        m_state.renderShadow
                            ? m_shadowMapFBO.depthTextureAttachment()
                            : nullptr);

    if (m_state.renderGrid)
        m_gridRenderer->render(m_camera);

    m_FBO.unbind();

    m_FBO.bindRead();
    m_intermediateFBO.bindDraw();
    glBlitFramebuffer(0, 0, m_viewportWidth, m_viewportHeight, 0, 0,
                      m_viewportWidth, m_viewportHeight, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    m_intermediateFBO.unbind();
}

void Context::controlPanel()
{
    ImGui::Begin("Control", nullptr, ImGuiWindowFlags_NoMove);

    if (ImGui::BeginListBox("Models"))
    {
        for (int i = 0; i < static_cast<int>(m_models.size()); ++i)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(
                    m_modelData[m_modelIndexMap[i]]->info.modelName.c_str(),
                    m_selectedModelIndex == i))
                m_selectedModelIndex = i;
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    if (m_selectedModelIndex != -1)
    {
        if (ImGui::Button("Remove"))
        {
            removeModel(m_selectedModelIndex);
            m_selectedModelIndex = -1;
        }
    }

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Physics: %.3f ms", m_profiler.query("Physics") * 1000.f);
    ImGui::Text("Model update: %.3f ms",
                m_profiler.query("Model update") * 1000.f);
    ImGui::Text("Render: %.3f ms", m_profiler.query("Render") * 1000.f);
    ImGui::Text("Total: %.3f ms", m_profiler.queryTotal() * 1000.f);

    if (ImGui::Button(m_state.paused ? "Resume" : "Pause"))
    {
        if (m_state.paused)
            for (auto &animator : m_animators)
                animator->resume();
        else
            for (auto &animator : m_animators)
                animator->pause();
        m_state.paused = !m_state.paused;
    }

    if (ImGui::Checkbox("Physics", &m_state.physicsEnabled))
    {
        if (m_state.physicsEnabled)
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

    ImGui::Combo("Physics FPS", &m_state.physicsFPSSelection,
                 "60\000120\000240");

    if (ImGui::Button("Reset"))
    {
        for (auto &animator : m_animators)
            animator->reset();
    }

    if (ImGui::SliderFloat3("Gravity", &m_state.gravity.x, -10.f, 10.f))
        m_physicsWorld.setGravity(m_state.gravity);

    ImGui::ColorEdit4("Clear color", &m_state.clearColor.x);

    if (ImGui::Checkbox("Ortho", &m_state.ortho))
        m_camera.projType = m_state.ortho
                                ? glmmd::CameraProjectionType::Orthographic
                                : glmmd::CameraProjectionType::Perspective;

    if (ImGui::Checkbox("Render edge", &m_state.renderEdge))
    {
        for (auto &renderer : m_modelRenderers)
            if (m_state.renderEdge)
                renderer.renderFlag() |= glmmd::MODEL_RENDER_FLAG_EDGE;
            else
                renderer.renderFlag() &= ~glmmd::MODEL_RENDER_FLAG_EDGE;
    }

    if (ImGui::Checkbox("Render ground shadow", &m_state.renderGroundShadow))
    {
        for (auto &renderer : m_modelRenderers)
            if (m_state.renderGroundShadow)
                renderer.renderFlag() |= glmmd::MODEL_RENDER_FLAG_GROUND_SHADOW;
            else
                renderer.renderFlag() &=
                    ~glmmd::MODEL_RENDER_FLAG_GROUND_SHADOW;
    }

    ImGui::Checkbox("Render shadow", &m_state.renderShadow);

    if (ImGui::Checkbox("Render axes", &m_state.renderAxes))
    {
        m_gridRenderer->showAxes = m_state.renderAxes;
    }
    ImGui::Checkbox("Render grid", &m_state.renderGrid);

    if (ImGui::SliderFloat3("Light direction", &m_lighting.direction.x, -1.f,
                            1.f))
        m_lighting.direction = glm::normalize(m_lighting.direction);

    ImGui::End();
}

void Context::run()
{
    auto &io = ImGui::GetIO();

    m_profiler.push("Physics");
    m_profiler.push("Model update");
    m_profiler.push("Render");

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.f, 0.f, 0.f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        dockspace();

        float deltaTime = io.DeltaTime;

        m_profiler.startFrame();

        {
            m_profiler.start("Physics");
            const int physicsFPS[3]{60, 120, 240};
            m_physicsWorld.update(
                deltaTime, 10, 1.f / physicsFPS[m_state.physicsFPSSelection]);
            m_profiler.stop("Physics");
        }

        {
            m_profiler.start("Model update");
            if (m_state.shouldUpdateModels)
                updateModels();
            m_profiler.stop("Model update");
        }

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Viewport", nullptr,
                         ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBackground);

            updateViewportSize();

            if (ImGui::IsWindowFocused())
                handleInput(deltaTime);

            m_profiler.start("Render");
            render();
            m_profiler.stop("Render");

            ImGui::Image(m_intermediateFBO.colorTextureAttachment()->id(),
                         ImVec2(static_cast<float>(m_viewportWidth),
                                static_cast<float>(m_viewportHeight)),
                         ImVec2(0, 1), ImVec2(1, 0));
            ImGui::End();
            ImGui::PopStyleVar();
        }

        m_state.shouldUpdateModels = !m_state.paused || m_state.physicsEnabled;

        controlPanel();

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
    m_gridRenderer.reset();

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
