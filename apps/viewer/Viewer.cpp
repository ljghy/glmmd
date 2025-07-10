#include <algorithm>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <ImGuiFileDialog.h>

#include <glmmd/core/FixedPoseMotion.h>
#include <glmmd/core/ParallelForEach.h>
#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>
#include <glmmd/files/VmdFileLoader.h>
#include <glmmd/files/VpdFileLoader.h>

#include "PathConv.h"
#include "Viewer.h"

void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

void dropCallback(GLFWwindow *window, int count, const char **paths)
{
    auto *viewer = (Viewer *)glfwGetWindowUserPointer(window);
    for (int i = 0; i < count; ++i)
    {
        auto path = u8stringToPath(paths[i]);

        if (path.extension() == ".pmx")
        {
            if (viewer->loadModel(path))
                viewer->m_state.selectedModelIndex =
                    static_cast<int>(viewer->m_models.size()) - 1;
        }
        else if (path.extension() == ".vmd")
            viewer->loadMotion(path, viewer->m_state.selectedModelIndex);
        else if (path.extension() == ".vpd")
            viewer->loadPose(path, viewer->m_state.selectedModelIndex);
        else
            std::cout << "Unsupported file type: " << pathToU8string(path)
                      << std::endl;
    }
}

Viewer::Viewer(const std::filesystem::path &executableDir)
    : m_executableDir(executableDir)
{
    std::filesystem::path initFilePath = executableDir / "init.json";

    if (std::filesystem::exists(initFilePath))
    {
        m_initData = parseJsonFile(initFilePath);
    }
    else
    {
        std::cout << "init.json not found, using default settings."
                  << std::endl;
        m_initData = JsonNode{{"MSAA"_key, 4}};
    }

    initState();

    initWindow();
    initImGui();
    initFBO();
    loadResources();

    m_gridRenderer = std::make_unique<InfiniteGridRenderer>();

    initCamera();
    initMainLight();
}

void Viewer::initState()
{
    m_state.showControlPanel = true;
    m_state.showProfiler     = true;
    m_state.showProgress     = true;

    m_state.selectedModelIndex  = -1;
    m_state.selectedMotionIndex = -1;

    m_state.paused    = true;
    m_state.startTime = m_state.pauseTime = std::chrono::steady_clock::now();

    m_state.physicsEnabled      = false;
    m_state.physicsFPSSelection = 1;
    m_state.physicsSubsteps     = 10;

    m_state.gravity = glm::vec3(0.f, -9.8f, 0.f);

    m_state.clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);

    m_state.ortho        = m_camera.projType == glmmd::Camera::Orthographic;
    m_state.renderEdge   = true;
    m_state.renderShadow = true;
    m_state.renderGroundShadow = true;
    m_state.renderAxes         = true;
    m_state.renderGrid         = true;
    m_state.wireframe          = false;
    m_state.lockCamera         = true;

    m_state.shadowDistance = 50.f;

    m_state.lastModelPath  = ".";
    m_state.lastMotionPath = ".";
    m_state.lastPosePath   = ".";
}

void Viewer::initCamera()
{
    m_camera.projType = glmmd::Camera::Perspective;
    m_camera.target   = glm::vec3(0.f, 8.f, 0.f);
    m_camera.setRotation(glm::radians(glm::vec3(-10.f, 0.f, 0.f)));
    m_camera.distance = 30.f;
    m_camera.fov      = glm::radians(45.f);
    m_camera.zNear    = 0.1f;
    m_camera.zFar     = 2000.f;
    m_camera.width    = 40.f;

    m_camera.resize(m_viewportWidth, m_viewportHeight);
    m_camera.update();
}

void Viewer::initMainLight()
{
    m_mainDirectionalLight.direction =
        glm::normalize(glm::vec3(-1.f, -2.f, 1.f));
    m_mainDirectionalLight.color        = glm::vec3(0.6f);
    m_mainDirectionalLight.ambientColor = glm::vec3(1.f);

    m_mainDirectionalLight.direction =
        glm::normalize(glm::vec3(-1.f, -2.f, 1.f));
}

void Viewer::initWindow()
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
    if (m_window == nullptr)
    {
        throw std::runtime_error("Failed to create window.");
    }
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetDropCallback(m_window, dropCallback);
    glfwSetWindowUserPointer(m_window, this);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD.");
    }
}

void Viewer::initImGui()
{
    const char *glsl_version = "#version 330";

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

    style.ChildBorderSize  = 1.f;
    style.FrameBorderSize  = 0.f;
    style.PopupBorderSize  = 1.f;
    style.WindowBorderSize = 0.f;
    style.FrameRounding    = 2.f;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    if (m_initData.contains("UIScale") &&
        m_initData.get<float>("UIScale") > 0.f)
    {
        m_uiScale = m_initData.get<float>("UIScale");
    }
    else
    {
        float xScale, yScale;
        glfwGetWindowContentScale(m_window, &xScale, &yScale);
        m_uiScale = std::max(xScale, yScale);
    }

    style.ScaleAllSizes(m_uiScale);

    std::filesystem::path defaultFontPath =
        m_executableDir / "font" / "NotoSansCJK-Bold.ttc";
    std::ifstream fontFile(defaultFontPath, std::ios::binary);
    if (!fontFile)
    {
        std::cerr << "Failed to load default font.\n";
    }
    else
    {
        std::vector<char> fontData(std::istreambuf_iterator<char>(fontFile),
                                   std::istreambuf_iterator<char>{});

        static ImFontGlyphRangesBuilder range;
        range.AddRanges(io.Fonts->GetGlyphRangesJapanese());
        range.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
        static ImVector<ImWchar> glyphRanges;
        range.BuildRanges(&glyphRanges);

        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;

        auto font = io.Fonts->AddFontFromMemoryTTF(
            fontData.data(), static_cast<int>(fontData.size()),
            18.f * m_uiScale, &cfg, glyphRanges.Data);
        if (font == nullptr)
            std::cerr << "Failed to load default font.\n";
    }
}

void Viewer::initFBO()
{
    glfwGetWindowSize(m_window, &m_viewportWidth, &m_viewportHeight);

    int samples = m_initData.get<int>("MSAA", 4);

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
    rboInfo.internalFmt = GL_DEPTH_COMPONENT24;
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
    m_shadowMapWidth             = m_initData.get<int>("ShadowMapWidth", 2048);
    m_shadowMapHeight            = m_initData.get<int>("ShadowMapHeight", 2048);
    shadowMapTexInfo.width       = m_shadowMapWidth;
    shadowMapTexInfo.height      = m_shadowMapHeight;
    shadowMapTexInfo.internalFmt = GL_DEPTH_COMPONENT;
    shadowMapTexInfo.dataFmt     = GL_DEPTH_COMPONENT;
    shadowMapTexInfo.dataType    = GL_FLOAT;
    shadowMapTexInfo.wrapModeS   = GL_CLAMP_TO_BORDER;
    shadowMapTexInfo.wrapModeT   = GL_CLAMP_TO_BORDER;
    shadowMapTexInfo.minFilterMode = GL_NEAREST;
    shadowMapTexInfo.magFilterMode = GL_NEAREST;
    std::unique_ptr<ogl::Texture2D> shadowMapTex =
        std::make_unique<ogl::Texture2D>(shadowMapTexInfo);
    shadowMapTex->bind();
    float borderColor[] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    m_shadowMapFBO.attachDepthTexture(std::move(shadowMapTex));

    if (!m_shadowMapFBO.isComplete())
        throw std::runtime_error("Failed to create shadow map FBO.");
}

bool Viewer::loadModel(const std::filesystem::path &path)
{
    std::shared_ptr<glmmd::ModelData> modelData(nullptr);
    std::vector<ogl::Texture2D>       gpuTextures;
    try
    {
        modelData = glmmd::loadPmxFile(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    if (!modelData)
        return false;

    std::cout << "Model loaded from: " << pathToU8string(path) << '\n';
    std::cout << "Name: " << modelData->info.modelName << '\n';
    std::cout << "Comment: " << modelData->info.comment << '\n';
    std::cout << std::endl;

    auto &renderer = m_modelRenderers.emplace_back(
        std::make_unique<ModelRenderer>(modelData));

    uint32_t renderFlag = MODEL_RENDER_FLAG_MESH;

    if (m_state.renderEdge)
        renderFlag |= MODEL_RENDER_FLAG_EDGE;
    if (m_state.renderGroundShadow)
        renderFlag |= MODEL_RENDER_FLAG_GROUND_SHADOW;
    renderer->renderFlag() = renderFlag;

    auto &model =
        m_models.emplace_back(std::make_unique<glmmd::Model>(modelData));
    m_motions.emplace_back(std::make_unique<BlendedMotion>(modelData));

    if (m_state.physicsEnabled)
        m_physicsWorld.setupModelPhysics(*model);

    return true;
}

void Viewer::removeModel(size_t i)
{
    m_physicsWorld.clearModelPhysics(*m_models[i]);
    m_motions.erase(m_motions.begin() + i);
    m_modelRenderers.erase(m_modelRenderers.begin() + i);
    m_models.erase(m_models.begin() + i);
}

void Viewer::loadMotion(const std::filesystem::path &path, size_t modelIndex,
                        const JsonNode &config)
{
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

    auto filename = path.filename().u8string();

    if (vmdData->isCameraMotion())
    {
        m_cameraMotion = std::make_unique<glmmd::CameraMotion>(
            vmdData->toCameraMotion(loop));

        std::cout << "Camera motion data loaded from: " << pathToU8string(path)
                  << '\n';
        std::cout << "Duration: " << m_cameraMotion->duration() << " s\n";
        std::cout << std::endl;
    }
    else
    {
        if (modelIndex >= m_models.size())
        {
            std::cerr << "Invalid model index.\n";
            return;
        }
        auto clip = std::make_shared<glmmd::FixedMotionClip>(
            vmdData->toFixedMotionClip(m_models[modelIndex]->data(), loop));

        std::string label(filename.begin(), filename.end());
        m_motions[modelIndex]->addMotion(label, clip);

        std::cout << "Motion data loaded from: " << pathToU8string(path)
                  << '\n';
        std::cout << "Created on: "
                  << glmmd::codeCvt<glmmd::ShiftJIS, glmmd::UTF8>(
                         vmdData->modelName)
                  << '\n';
        std::cout << "Duration: " << clip->duration() << " s\n";
        std::cout << std::endl;
    }
}

void Viewer::loadPose(const std::filesystem::path &path, size_t modelIndex)
{
    if (modelIndex >= m_models.size())
    {
        std::cerr << "Invalid model index.\n";
        return;
    }

    std::shared_ptr<glmmd::VpdData> vpdData(nullptr);
    try
    {
        vpdData = glmmd::loadVpdFile(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    if (!vpdData)
        return;

    auto pose = vpdData->toModelPose(m_models[modelIndex]->dataPtr());

    auto        filename = path.filename().u8string();
    std::string label(filename.begin(), filename.end());
    m_motions[modelIndex]->addMotion(
        label, std::make_shared<glmmd::FixedPoseMotion>(std::move(pose)));

    std::cout << "Pose data loaded from: " << pathToU8string(path) << '\n';
    std::cout << "Created on: "
              << glmmd::codeCvt<glmmd::ShiftJIS, glmmd::UTF8>(
                     vpdData->modelName)
              << '\n';
    std::cout << std::endl;
}

void Viewer::loadResources()
{
    if (m_initData.contains("models"))
    {
        for (const auto &modelNode : m_initData["models"].arr())
            loadModel(modelNode.get<std::filesystem::path>("filename"));
        if (!m_models.empty())
            m_state.selectedModelIndex = 0;
    }

    if (m_initData.contains("motions"))
        for (const auto &motionNode : m_initData["motions"].arr())
            loadMotion(motionNode.get<std::filesystem::path>("filename"),
                       motionNode.get<size_t>("model"), motionNode);

    if (m_initData.contains("poses"))
        for (const auto &poseNode : m_initData["poses"].arr())
            loadPose(poseNode.get<std::filesystem::path>("filename"),
                     poseNode.get<size_t>("model"));
}

void Viewer::handleInput(float deltaTime)
{
    auto &io = ImGui::GetIO();

    ImVec2          mouseDelta  = io.MouseDelta;
    constexpr float sensitivity = glm::radians(0.1f);

    if (io.MouseDown[1])
        m_camera.rotate(-mouseDelta.x * sensitivity,
                        -mouseDelta.y * sensitivity);

    if (io.MouseDown[2])
    {
        float     vel = 5.f * glm::tan(m_camera.fov * 0.5f);
        glm::vec3 translation =
            -vel * mouseDelta.x * deltaTime * m_camera.right() +
            vel * mouseDelta.y * deltaTime * m_camera.up();
        m_camera.target += translation;
    }

    if (m_camera.projType == glmmd::Camera::Perspective)
    {
        m_camera.fov -= glm::radians(5.f) * io.MouseWheel;
        m_camera.fov =
            glm::clamp(m_camera.fov, glm::radians(1.f), glm::radians(120.f));
    }
    else
    {
        m_camera.width -= 5.f * io.MouseWheel;
        m_camera.width = glm::clamp(m_camera.width, 1.f, 100.f);
    }

    constexpr float vel = 25.f;

    glm::vec3 translation(0.f);
    if (ImGui::IsKeyDown(ImGuiKey_W))
        translation += vel * deltaTime * m_camera.front();
    if (ImGui::IsKeyDown(ImGuiKey_S))
        translation -= vel * deltaTime * m_camera.front();
    if (ImGui::IsKeyDown(ImGuiKey_A))
        translation -= vel * deltaTime * m_camera.right();
    if (ImGui::IsKeyDown(ImGuiKey_D))
        translation += vel * deltaTime * m_camera.right();

    m_camera.target += translation;
}

void Viewer::updateModelPose(size_t i)
{
    auto &model = m_models[i];
    model->resetLocalPose();
    m_motions[i]->getLocalPose(m_state.progress, model->pose());
    model->solvePose();
}

void Viewer::menuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load model"))
            {
                IGFD::FileDialogConfig config;
                config.path = m_state.lastModelPath;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "LoadModelDlg", "Load model", ".pmx", config);
            }

            if (ImGui::MenuItem("Load motion"))
            {
                IGFD::FileDialogConfig config;
                config.path = m_state.lastMotionPath;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "LoadMotionDlg", "Load motion", ".vmd", config);
            }

            if (m_state.selectedModelIndex != -1 &&
                ImGui::MenuItem("Load pose"))
            {
                IGFD::FileDialogConfig config;
                config.path = m_state.lastPosePath;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "LoadPoseDlg", "Load pose", ".vpd", config);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(m_window, true);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Control Panel", nullptr,
                            &m_state.showControlPanel);
            ImGui::MenuItem("Profiler", nullptr, &m_state.showProfiler);
            ImGui::MenuItem("Progress", nullptr, &m_state.showProgress);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Viewer::dockspace()
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

    menuBar();

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

        ImGuiID viewportDockId = dockspaceId;
        ImGuiID controlDockId  = ImGui::DockBuilderSplitNode(
            viewportDockId, ImGuiDir_Left, 0.2f, nullptr, &viewportDockId);
        ImGuiID profilerDockId = ImGui::DockBuilderSplitNode(
            controlDockId, ImGuiDir_Down, 0.2f, nullptr, &controlDockId);
        ImGuiID progressDockId = ImGui::DockBuilderSplitNode(
            viewportDockId, ImGuiDir_Down, 0.12f, nullptr, &viewportDockId);

        ImGui::DockBuilderDockWindow("Control", controlDockId);
        ImGui::DockBuilderDockWindow("Profiler", profilerDockId);
        ImGui::DockBuilderDockWindow("Progress", progressDockId);
        ImGui::DockBuilderDockWindow("Viewport", viewportDockId);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void Viewer::loadModelDialog()
{
    if (ImGuiFileDialog::Instance()->Display("LoadModelDlg",
                                             ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoDocking))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            if (loadModel(u8stringToPath(
                    ImGuiFileDialog::Instance()->GetFilePathName())))
                m_state.selectedModelIndex =
                    static_cast<int>(m_models.size()) - 1;
            m_state.lastModelPath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void Viewer::loadMotionDialog()
{
    if (ImGuiFileDialog::Instance()->Display("LoadMotionDlg",
                                             ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoDocking))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            loadMotion(ImGuiFileDialog::Instance()->GetFilePathName(),
                       m_state.selectedModelIndex);
            m_state.lastMotionPath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void Viewer::loadPoseDialog()
{
    if (ImGuiFileDialog::Instance()->Display("LoadPoseDlg",
                                             ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoDocking))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            loadPose(ImGuiFileDialog::Instance()->GetFilePathName(),
                     m_state.selectedModelIndex);
            m_state.lastPosePath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void Viewer::updateModels()
{
    glmmd::parallelForEach(m_models.begin(), m_models.end(),
                           [&](const auto &model)
                           {
                               auto i = &model - m_models.data();
                               updateModelPose(i);
                               m_modelRenderers[i]->renderData().init();
                               m_models[i]->pose().applyToRenderData(
                                   m_modelRenderers[i]->renderData());
                           });
}

void Viewer::updateCameraMotion()
{
    if (!m_cameraMotion)
        return;

    m_cameraMotion->updateCamera(m_state.progress, m_camera);
}

void Viewer::updateViewportSize()
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

void Viewer::render()
{

    for (const auto &renderer : m_modelRenderers)
        renderer->fillBuffers();

    // Render shadow map

    if (m_state.renderShadow)
    {
        m_shadowMapFBO.bind();
        glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
        glClear(GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        for (const auto &renderer : m_modelRenderers)
            renderer->renderShadowMap(m_mainDirectionalLight);
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

    if (m_state.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (const auto &renderer : m_modelRenderers)
        renderer->render(m_camera, m_mainDirectionalLight,
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

void Viewer::play()
{
    m_state.startTime += std::chrono::steady_clock::now() - m_state.pauseTime;
}

void Viewer::pause() { m_state.pauseTime = std::chrono::steady_clock::now(); }

void Viewer::resetProgress()
{
    m_state.startTime = std::chrono::steady_clock::now();
    if (m_state.paused)
        m_state.pauseTime = m_state.startTime;
}

float Viewer::getProgress() const
{
    if (m_state.paused)
        return std::chrono::duration<float>(m_state.pauseTime -
                                            m_state.startTime)
            .count();
    else
        return std::chrono::duration<float>(std::chrono::steady_clock::now() -
                                            m_state.startTime)
            .count();
}

void Viewer::setProgress(float progress)
{
    auto now = std::chrono::steady_clock::now();
    m_state.startTime =
        std::chrono::time_point_cast<std::chrono::steady_clock::duration>(
            now -
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<float>(progress)));
    if (m_state.paused)
        m_state.pauseTime = now;

    m_state.progress = progress;
}

void Viewer::progress()
{
    ImGui::Begin("Progress", nullptr, ImGuiWindowFlags_NoMove);

    float duration = 0.f;
    for (const auto &motion : m_motions)
        duration = std::max(duration, motion->duration());
    if (m_cameraMotion)
        duration = std::max(duration, m_cameraMotion->duration());

    if (ImGui::SliderFloat("##Progress", &m_state.progress, 0.f, duration, ""))
        setProgress(m_state.progress);

    ImGui::SameLine();
    ImGui::Text("%.2f / %.2f", m_state.progress, duration);

    if (ImGui::Button(m_state.paused ? "Play" : "Pause"))
    {
        if (m_state.paused)
            play();
        else
            pause();

        m_state.paused = !m_state.paused;
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        resetProgress();

    ImGui::End();
}

void Viewer::modelList()
{
    if (ImGui::BeginListBox("Models"))
    {
        for (int i = 0; i < static_cast<int>(m_models.size()); ++i)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(m_models[i]->data().info.modelName.c_str(),
                                  m_state.selectedModelIndex == i))
                m_state.selectedModelIndex = i;
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    if (m_state.selectedModelIndex != -1)
    {
        if (ImGui::Button("Remove##Model"))
        {
            removeModel(m_state.selectedModelIndex);
            if (m_state.selectedModelIndex > 0)
                --m_state.selectedModelIndex;
            else
                m_state.selectedModelIndex = m_models.empty() ? -1 : 0;
        }

        if (m_state.selectedModelIndex == -1)
            return;

        ImGui::SameLine();
        if (ImGui::Button("Up##Model") && m_state.selectedModelIndex > 0)
        {
            std::swap(m_models[m_state.selectedModelIndex],
                      m_models[m_state.selectedModelIndex - 1]);
            std::swap(m_modelRenderers[m_state.selectedModelIndex],
                      m_modelRenderers[m_state.selectedModelIndex - 1]);
            std::swap(m_motions[m_state.selectedModelIndex],
                      m_motions[m_state.selectedModelIndex - 1]);
            --m_state.selectedModelIndex;
        }
        ImGui::SameLine();
        if (ImGui::Button("Down##Model") &&
            static_cast<size_t>(m_state.selectedModelIndex + 1) <
                m_models.size())
        {
            std::swap(m_models[m_state.selectedModelIndex],
                      m_models[m_state.selectedModelIndex + 1]);
            std::swap(m_modelRenderers[m_state.selectedModelIndex],
                      m_modelRenderers[m_state.selectedModelIndex + 1]);
            std::swap(m_motions[m_state.selectedModelIndex],
                      m_motions[m_state.selectedModelIndex + 1]);
            ++m_state.selectedModelIndex;
        }
        ImGui::SameLine();
        bool hideModel =
            m_modelRenderers[m_state.selectedModelIndex]->renderFlag() &
            MODEL_RENDER_FLAG_HIDE;
        if (ImGui::Checkbox("Hide##Model", &hideModel))
        {
            if (hideModel)
                m_modelRenderers[m_state.selectedModelIndex]->renderFlag() |=
                    MODEL_RENDER_FLAG_HIDE;
            else
                m_modelRenderers[m_state.selectedModelIndex]->renderFlag() &=
                    ~MODEL_RENDER_FLAG_HIDE;
        }

        const auto &motion = m_motions[m_state.selectedModelIndex];
        if (!motion->empty() && ImGui::BeginListBox("Motions"))
        {
            for (int i = 0; i < static_cast<int>(motion->labels().size()); ++i)
            {
                ImGui::PushID(i);
                if (ImGui::Selectable(motion->labels()[i].c_str(),
                                      m_state.selectedMotionIndex == i))
                    m_state.selectedMotionIndex = i;
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }

        if (m_state.selectedMotionIndex != -1 &&
            m_state.selectedMotionIndex < static_cast<int>(motion->size()))
        {
            if (ImGui::Button("Remove##Motion"))
            {
                motion->removeMotion(m_state.selectedMotionIndex);
                m_state.selectedMotionIndex = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Up##Motion"))
            {
                if (motion->moveUp(m_state.selectedMotionIndex))
                    --m_state.selectedMotionIndex;
            }
            ImGui::SameLine();
            if (ImGui::Button("Down##Motion"))
            {
                if (motion->moveDown(m_state.selectedMotionIndex))
                    ++m_state.selectedMotionIndex;
            }
        }
    }
}

void Viewer::controlPanel()
{
    ImGui::Begin("Control", nullptr, ImGuiWindowFlags_NoMove);

    modelList();

    if (ImGui::TreeNode("Camera"))
    {
        if (ImGui::Checkbox("Ortho", &m_state.ortho))
            m_camera.projType = m_state.ortho ? glmmd::Camera::Orthographic
                                              : glmmd::Camera::Perspective;

        float fov = glm::degrees(m_camera.fov);
        if (ImGui::SliderFloat("FOV", &fov, 1.f, 120.f))
            m_camera.fov = glm::radians(fov);

        ImGui::InputFloat("Near", &m_camera.zNear);
        ImGui::InputFloat("Far", &m_camera.zFar);

        if (m_cameraMotion && ImGui::Button("Clear camera motion"))
        {
            m_cameraMotion.reset();
            initCamera();
        }
        if (ImGui::Button("Reset##Camera"))
            initCamera();

        ImGui::Checkbox("Lock", &m_state.lockCamera);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Physics"))
    {
        if (ImGui::Checkbox("Physics", &m_state.physicsEnabled))
        {
            if (m_state.physicsEnabled)
            {
                for (auto &model : m_models)
                    m_physicsWorld.setupModelPhysics(*model, true);
            }
            else
            {
                for (auto &model : m_models)
                    m_physicsWorld.clearModelPhysics(*model);
            }
        }

        ImGui::Combo("Physics FPS", &m_state.physicsFPSSelection,
                     "60\000120\000240\000");

        if (ImGui::InputInt("Substeps", &m_state.physicsSubsteps))
            m_state.physicsSubsteps = std::max(1, m_state.physicsSubsteps);

        if (ImGui::SliderFloat3("Gravity", &m_state.gravity.x, -10.f, 10.f))
            m_physicsWorld.setGravity(m_state.gravity);

        if (ImGui::Button("Reset"))
        {
            m_state.gravity = glm::vec3(0.f, -9.8f, 0.f);
            m_physicsWorld.setGravity(m_state.gravity);

            m_state.physicsSubsteps = 10;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Render"))
    {

        ImGui::ColorEdit4("Clear color", &m_state.clearColor.x);

        if (ImGui::Checkbox("Render edge", &m_state.renderEdge))
        {
            for (auto &renderer : m_modelRenderers)
                if (m_state.renderEdge)
                    renderer->renderFlag() |= MODEL_RENDER_FLAG_EDGE;
                else
                    renderer->renderFlag() &= ~MODEL_RENDER_FLAG_EDGE;
        }

        if (ImGui::Checkbox("Render ground shadow",
                            &m_state.renderGroundShadow))
        {
            for (auto &renderer : m_modelRenderers)
                if (m_state.renderGroundShadow)
                    renderer->renderFlag() |= MODEL_RENDER_FLAG_GROUND_SHADOW;
                else
                    renderer->renderFlag() &= ~MODEL_RENDER_FLAG_GROUND_SHADOW;
        }

        ImGui::Checkbox("Render shadow", &m_state.renderShadow);

        if (ImGui::Checkbox("Render axes", &m_state.renderAxes))
        {
            m_gridRenderer->showAxes = m_state.renderAxes;
        }
        ImGui::Checkbox("Render grid", &m_state.renderGrid);

        ImGui::Checkbox("Wireframe", &m_state.wireframe);

        if (ImGui::SliderFloat3("Light direction",
                                &m_mainDirectionalLight.direction.x, -1.f, 1.f))
            m_mainDirectionalLight.direction =
                glm::normalize(m_mainDirectionalLight.direction);

        ImGui::ColorEdit3("Light color", &m_mainDirectionalLight.color.x);

        ImGui::ColorEdit3("Ambient color",
                          &m_mainDirectionalLight.ambientColor.x);

        ImGui::InputFloat("Shadow distance", &m_state.shadowDistance);

        ImGui::TreePop();
    }

    ImGui::End();
}

void Viewer::profiler()
{
    ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoMove);

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Physics: %.3f ms", m_profiler.averageTime("Physics"));
    ImGui::Text("Model update: %.3f ms",
                m_profiler.averageTime("Model update"));
    ImGui::Text("Render: %.3f ms", m_profiler.averageTime("Render"));
    ImGui::Text("Total: %.3f ms", m_profiler.totalTime());

    ImGui::End();
}

void Viewer::run()
{
    auto &io = ImGui::GetIO();

    m_profiler.add("Physics");
    m_profiler.add("Model update");
    m_profiler.add("Render");

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        dockspace();
        loadModelDialog();
        loadMotionDialog();
        loadPoseDialog();

        float deltaTime = io.DeltaTime;

        m_state.progress = getProgress();

        m_profiler.startFrame();

        {
            m_profiler.start("Physics");
            const int physicsFPS[3]{60, 120, 240};
            m_physicsWorld.update(deltaTime, m_state.physicsSubsteps,
                                  1.f /
                                      physicsFPS[m_state.physicsFPSSelection]);
            m_profiler.stop("Physics");
        }

        {
            m_profiler.start("Model update");
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

            if (!m_state.paused || m_state.lockCamera)
                updateCameraMotion();

            m_camera.update();

            glm::vec3 cameraFrustumCorners[8];
            m_camera.getFrustumCorners(cameraFrustumCorners, m_camera.zNear,
                                       m_camera.zNear + m_state.shadowDistance);

            m_mainDirectionalLight.updateFrustum(8, cameraFrustumCorners);
            m_mainDirectionalLight.update();

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

        if (m_state.showProgress)
            progress();

        if (m_state.showControlPanel)
            controlPanel();

        m_profiler.endFrame();
        if (m_state.showProfiler)
            profiler();

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

Viewer::~Viewer()
{
    m_gridRenderer.reset();

    ModelRenderer::releaseSharedToonTextures();

    m_modelRenderers.clear();
    m_FBO.destroy();
    m_intermediateFBO.destroy();
    m_shadowMapFBO.destroy();

    ImGui::GetIO().Fonts->Clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
