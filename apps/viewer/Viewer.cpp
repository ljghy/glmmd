#include <algorithm>
#include <iostream>
#include <optional>
#include <utility>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <ImGuiFileDialog.h>

#include <glmmd/core/ParallelForEach.h>
#include <glmmd/core/PoseMotion.h>
#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>
#include <glmmd/files/VmdFileLoader.h>
#include <glmmd/files/VpdFileLoader.h>

#include "PathConv.h"
#include "Viewer.h"

void framebufferSizeCallback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

void dropCallback(GLFWwindow *window, int count, const char **paths) {
  auto *viewer = (Viewer *)glfwGetWindowUserPointer(window);
  for (int i = 0; i < count; ++i) {
    auto path = u8stringToPath(paths[i]);

    if (path.extension() == ".pmx") {
      if (viewer->loadModel(path))
        viewer->m_state.selectedModelIndex =
            static_cast<int>(viewer->m_models.size()) - 1;
    } else if (path.extension() == ".vmd" || path.extension() == ".vpd")
      viewer->loadMotion(path, viewer->m_state.selectedModelIndex);
    else
      std::cout << "Unsupported file type: " << pathToU8string(path)
                << std::endl;
  }
}

Viewer::Viewer(const std::filesystem::path &executableDir)
    : m_executableDir(executableDir) {
  std::filesystem::path initFilePath = executableDir / "init.json";

  if (std::filesystem::exists(initFilePath)) {
    m_initData = parseJsonFile(initFilePath);
  } else {
    std::cout << "init.json not found, using default settings." << std::endl;
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

void Viewer::initState() {
  m_state.showControlPanel = true;
  m_state.showProfiler = true;
  m_state.showProgress = true;

  m_state.selectedModelIndex = -1;
  m_state.selectedMotionIndex = -1;

  m_state.paused = true;
  m_state.startTime = m_state.pauseTime = std::chrono::steady_clock::now();

  m_state.physicsEnabled = false;
  m_state.physicsFPSSelection = 0;
  m_state.physicsSubsteps = 10;

  m_state.gravity = glm::vec3(0.f, -9.8f, 0.f);

  m_state.clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);

  m_state.ortho = m_camera.projType == glmmd::Camera::Orthographic;
  m_state.renderEdge = true;
  m_state.renderShadow = true;
  m_state.renderGroundShadow = true;
  m_state.renderAxes = true;
  m_state.renderGrid = true;
  m_state.wireframe = false;
  m_state.lockCamera = true;

  m_state.lastModelPath = ".";
  m_state.lastMotionPath = ".";
}

void Viewer::initCamera() {
  m_camera.projType = glmmd::Camera::Perspective;
  m_camera.target = glm::vec3(0.f, 8.f, 0.f);
  m_camera.setRotation(glm::radians(glm::vec3(-10.f, 0.f, 0.f)));
  m_camera.distance = 30.f;
  m_camera.fov = glm::radians(45.f);
  m_camera.zNear = 0.1f;
  m_camera.zFar = 2000.f;
  m_camera.width = 40.f;

  m_camera.resize(m_viewportWidth, m_viewportHeight);
  m_camera.update();
}

void Viewer::initMainLight() {
  m_mainDirectionalLight.direction =
      glm::normalize(glm::vec3(-0.5f, -1.f, 1.f));
  m_mainDirectionalLight.color = glm::vec3(1.f);
  m_mainDirectionalLight.ambientColor = glm::vec3(0.5f);
}

void Viewer::initWindow() {
  glfwSetErrorCallback([](int code, const char *description) {
    std::cerr << "GLFW error " << code << ": " << description << '\n';
  });
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW.");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

  int initWidth = m_initData.get<int>("WindowWidth", 1600);
  int initHeight = m_initData.get<int>("WindowHeight", 900);

  m_window = glfwCreateWindow(initWidth, initHeight, "Viewer", NULL, NULL);
  if (m_window == nullptr) {
    throw std::runtime_error(
        "Failed to create viewer window (OpenGL 4.1 core required).");
  }
  glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
  glfwSetDropCallback(m_window, dropCallback);
  glfwSetWindowUserPointer(m_window, this);

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Failed to initialize GLAD.");
  }
  if (!GLAD_GL_VERSION_4_1) {
    throw std::runtime_error("The viewer requires OpenGL 4.1 core or newer.");
  }
  std::cout << "OpenGL " << glGetString(GL_VERSION)
            << "\nRenderer: " << glGetString(GL_RENDERER) << std::endl;
}

void Viewer::initImGui() {
  const char *glsl_version = "#version 330";

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

  ImGui::StyleColorsClassic();

  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  style.ChildBorderSize = 1.f;
  style.FrameBorderSize = 0.f;
  style.PopupBorderSize = 1.f;
  style.WindowBorderSize = 0.f;
  style.FrameRounding = 2.f;

  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  std::filesystem::path defaultFontPath =
      m_executableDir / "font" / "NotoSansCJK-Bold.ttc";
  std::ifstream fontFile(defaultFontPath, std::ios::binary);
  if (!fontFile) {
    std::cerr << "Failed to load default font.\n";
  } else {
    m_fontData.assign(std::istreambuf_iterator<char>(fontFile),
                      std::istreambuf_iterator<char>{});
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;

    auto font = io.Fonts->AddFontFromMemoryTTF(
        m_fontData.data(), static_cast<int>(m_fontData.size()), 18.f, &cfg);

    if (font == nullptr)
      std::cerr << "Failed to load default font.\n";
  }
}

void Viewer::initFBO() {
  glfwGetWindowSize(m_window, &m_viewportWidth, &m_viewportHeight);

  m_sceneRenderer = std::make_unique<SceneRenderer>(
      m_viewportWidth, m_viewportHeight, m_initData.get<int>("MSAA", 4));

  m_shadowMap =
      std::make_unique<ShadowMap>(m_initData.get<int>("ShadowMapWidth", 4096),
                                  m_initData.get<int>("ShadowMapHeight", 4096));
}

bool Viewer::loadModel(const std::filesystem::path &path) {
  std::shared_ptr<glmmd::ModelData> modelData;
  std::vector<ogl::Texture2D> gpuTextures;
  try {
    modelData = std::make_shared<glmmd::ModelData>(glmmd::loadPmxFile(path));
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  if (!modelData)
    return false;

  std::cout << "Model loaded from: " << pathToU8string(path) << '\n';
  std::cout << "Name: " << modelData->info.modelName << '\n';
  std::cout << "Comment: " << modelData->info.comment << '\n';
  std::cout << std::endl;

  auto &renderer =
      m_modelRenderers.emplace_back(std::make_unique<ModelRenderer>(modelData));

  uint32_t renderFlag = MODEL_RENDER_FLAG_MESH;

  if (m_state.renderEdge)
    renderFlag |= MODEL_RENDER_FLAG_EDGE;
  if (m_state.renderGroundShadow)
    renderFlag |= MODEL_RENDER_FLAG_GROUND_SHADOW;
  renderer->renderFlag() = renderFlag;

  auto &model =
      m_models.emplace_back(std::make_unique<glmmd::Model>(modelData));
  m_motions.emplace_back(std::make_unique<MotionMixer>(modelData));

  if (m_state.physicsEnabled)
    m_physicsWorld.setupModelPhysics(*model);

  return true;
}

void Viewer::removeModel(size_t i) {
  m_physicsWorld.clearModelPhysics(*m_models[i]);
  m_motions.erase(m_motions.begin() + i);
  m_modelRenderers.erase(m_modelRenderers.begin() + i);
  m_models.erase(m_models.begin() + i);
}

void Viewer::loadMotion(const std::filesystem::path &path, size_t modelIndex,
                        const JsonNode &config) {
  const bool isPose = path.extension() == ".vpd";
  if (!isPose && path.extension() != ".vmd") {
    std::cerr << "Unsupported motion or pose file type: "
              << pathToU8string(path) << '\n';
    return;
  }

  try {
    const bool loop = config.get<bool>("loop", false);
    std::optional<glmmd::VmdData> vmdData;
    if (!isPose) {
      vmdData = glmmd::loadVmdFile(path);
      if (vmdData->isCameraMotion()) {
        m_cameraMotion = std::make_unique<glmmd::CameraMotion>(
            vmdData->toCameraMotion(loop));

        std::cout << "Camera motion data loaded from: " << pathToU8string(path)
                  << '\n';
        std::cout << "Duration: " << m_cameraMotion->duration() << " s\n";
        std::cout << std::endl;
        return;
      }
    }

    if (modelIndex >= m_models.size()) {
      std::cerr << "Select a model before loading a model motion or pose.\n";
      return;
    }

    const auto &modelData = *m_models[modelIndex]->data;
    std::shared_ptr<glmmd::Motion> motion;
    std::string modelName;
    if (isPose) {
      const auto vpdData = glmmd::loadVpdFile(path);
      motion = std::make_shared<glmmd::PoseMotion>(vpdData.toPose(modelData));
      modelName = vpdData.modelName;
    } else {
      motion = std::make_shared<glmmd::MotionClip>(
          vmdData->toMotionClip(modelData, loop));
      modelName = vmdData->modelName;
    }

    m_motions[modelIndex]->addMotion(pathToU8string(path.filename()), motion);

    std::cout << (isPose ? "Pose" : "Motion")
              << " data loaded from: " << pathToU8string(path) << '\n';
    std::cout << "Created on: "
              << glmmd::codeCvt<glmmd::ShiftJIS, glmmd::UTF8>(modelName)
              << '\n';
    if (!isPose)
      std::cout << "Duration: " << motion->duration() << " s\n";
    std::cout << std::endl;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}

void Viewer::loadResources() {
  if (m_initData.contains("models")) {
    for (const auto &modelNode : m_initData["models"].arr())
      loadModel(modelNode.get<std::filesystem::path>("path"));
    if (!m_models.empty())
      m_state.selectedModelIndex = 0;
  }

  if (m_initData.contains("motions"))
    for (const auto &motionNode : m_initData["motions"].arr()) {
      const auto path = motionNode.get<std::filesystem::path>("path");
      loadMotion(path, motionNode.get<size_t>("model"), motionNode);
    }
}

void Viewer::handleInput(float deltaTime) {
  auto &io = ImGui::GetIO();

  ImVec2 mouseDelta = io.MouseDelta;
  mouseDelta.x = -mouseDelta.x;

  constexpr float sensitivity = glm::radians(0.1f);

  if (io.MouseDown[1])
    m_camera.rotate(-mouseDelta.x * sensitivity, -mouseDelta.y * sensitivity);

  if (io.MouseDown[2]) {
    float vel = 5.f * glm::tan(m_camera.fov * 0.5f);
    glm::vec3 translation = -vel * mouseDelta.x * deltaTime * m_camera.right() +
                            vel * mouseDelta.y * deltaTime * m_camera.up();
    m_camera.target += translation;
  }

  if (m_camera.projType == glmmd::Camera::Perspective) {
    m_camera.fov -= glm::radians(5.f) * io.MouseWheel;
    m_camera.fov =
        glm::clamp(m_camera.fov, glm::radians(1.f), glm::radians(120.f));
  } else {
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
    translation += vel * deltaTime * m_camera.right();
  if (ImGui::IsKeyDown(ImGuiKey_D))
    translation -= vel * deltaTime * m_camera.right();

  m_camera.target += translation;
}

void Viewer::menuBar() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 6.f));
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load model")) {
        IGFD::FileDialogConfig config;
        config.path = m_state.lastModelPath;
        ImGuiFileDialog::Instance()->OpenDialog("LoadModelDlg", "Load model",
                                                ".pmx", config);
      }

      if (ImGui::MenuItem("Load motion / pose")) {
        IGFD::FileDialogConfig config;
        config.path = m_state.lastMotionPath;
        ImGuiFileDialog::Instance()->OpenDialog(
            "LoadMotionDlg", "Load motion / pose", "Motion or pose{.vmd,.vpd}",
            config);
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Exit"))
        glfwSetWindowShouldClose(m_window, true);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Control Panel", nullptr, &m_state.showControlPanel);
      ImGui::MenuItem("Profiler", nullptr, &m_state.showProfiler);
      ImGui::MenuItem("Progress", nullptr, &m_state.showProgress);
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
  ImGui::PopStyleVar();
}

void Viewer::dockspace() {
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
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

  menuBar();

  ImGuiID dockspaceId = ImGui::GetID("Dockspace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f),
                   ImGuiDockNodeFlags_PassthruCentralNode);

  static bool firstLoop = true;
  if (firstLoop) {
    firstLoop = false;
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

    ImGuiID viewportDockId = dockspaceId;
    ImGuiID controlDockId = ImGui::DockBuilderSplitNode(
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

void Viewer::loadModelDialog() {
  if (ImGuiFileDialog::Instance()->Display("LoadModelDlg",
                                           ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoDocking)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      if (loadModel(
              u8stringToPath(ImGuiFileDialog::Instance()->GetFilePathName())))
        m_state.selectedModelIndex = static_cast<int>(m_models.size()) - 1;
      m_state.lastModelPath = ImGuiFileDialog::Instance()->GetCurrentPath();
    }
    ImGuiFileDialog::Instance()->Close();
  }
}

void Viewer::loadMotionDialog() {
  if (ImGuiFileDialog::Instance()->Display("LoadMotionDlg",
                                           ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoDocking)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      loadMotion(u8stringToPath(ImGuiFileDialog::Instance()->GetFilePathName()),
                 m_state.selectedModelIndex);
      m_state.lastMotionPath = ImGuiFileDialog::Instance()->GetCurrentPath();
    }
    ImGuiFileDialog::Instance()->Close();
  }
}

void Viewer::updateModels() {
  glmmd::parallelForEach(
      m_models.begin(), m_models.end(), [&](const auto &model) {
        auto i = &model - m_models.data();

        model->pose.resetLocal();
        m_motions[i]->eval(m_state.progress, model->pose);
        model->solvePose();

        m_modelRenderers[i]->renderData().init();
        m_models[i]->pose.applyToRenderData(m_modelRenderers[i]->renderData());
      });
}

void Viewer::updateCameraMotion() {
  if (!m_cameraMotion)
    return;

  m_cameraMotion->updateCamera(m_state.progress, m_camera);
}

void Viewer::updateViewportSize() {
  int currentViewportWidth =
      static_cast<int>(std::round(ImGui::GetWindowWidth()));
  int currentViewportHeight =
      static_cast<int>(std::round(ImGui::GetWindowHeight()));
  if (currentViewportWidth > 0 && currentViewportHeight > 0 &&
      (m_viewportWidth != currentViewportWidth ||
       m_viewportHeight != currentViewportHeight)) {
    m_viewportWidth = currentViewportWidth;
    m_viewportHeight = currentViewportHeight;
    m_sceneRenderer->resize(m_viewportWidth, m_viewportHeight);

    m_camera.resize(m_viewportWidth, m_viewportHeight);
  }
}

void Viewer::render() {

  for (const auto &renderer : m_modelRenderers)
    renderer->fillBuffers();

  m_shadowMap->update(m_camera, m_mainDirectionalLight, m_modelRenderers);
  if (m_state.renderShadow)
    m_shadowMap->render(m_mainDirectionalLight, m_modelRenderers);

  m_sceneRenderer->render(m_modelRenderers,
                          m_state.renderGrid ? m_gridRenderer.get() : nullptr,
                          m_camera, m_mainDirectionalLight,
                          m_state.renderShadow ? m_shadowMap.get() : nullptr,
                          m_state.clearColor, m_state.wireframe);
}

void Viewer::play() {
  m_state.startTime += std::chrono::steady_clock::now() - m_state.pauseTime;
}

void Viewer::pause() { m_state.pauseTime = std::chrono::steady_clock::now(); }

void Viewer::resetProgress() {
  m_state.startTime = std::chrono::steady_clock::now();
  if (m_state.paused)
    m_state.pauseTime = m_state.startTime;
}

float Viewer::getProgress() const {
  if (m_state.paused)
    return std::chrono::duration<float>(m_state.pauseTime - m_state.startTime)
        .count();
  else
    return std::chrono::duration<float>(std::chrono::steady_clock::now() -
                                        m_state.startTime)
        .count();
}

void Viewer::setProgress(float progress) {
  auto now = std::chrono::steady_clock::now();
  m_state.startTime =
      std::chrono::time_point_cast<std::chrono::steady_clock::duration>(
          now - std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<float>(progress)));
  if (m_state.paused)
    m_state.pauseTime = now;

  m_state.progress = progress;
}

void Viewer::progress() {
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

  if (ImGui::Button(m_state.paused ? "Play" : "Pause")) {
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

void Viewer::modelList() {
  if (ImGui::BeginListBox("Models")) {
    for (int i = 0; i < static_cast<int>(m_models.size()); ++i) {
      ImGui::PushID(i);
      if (ImGui::Selectable(m_models[i]->data->info.modelName.c_str(),
                            m_state.selectedModelIndex == i))
        m_state.selectedModelIndex = i;
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }

  if (m_state.selectedModelIndex != -1) {
    if (ImGui::Button("Remove##Model")) {
      removeModel(m_state.selectedModelIndex);
      if (m_state.selectedModelIndex > 0)
        --m_state.selectedModelIndex;
      else
        m_state.selectedModelIndex = m_models.empty() ? -1 : 0;
    }

    if (m_state.selectedModelIndex == -1)
      return;

    ImGui::SameLine();
    if (ImGui::Button("Up##Model") && m_state.selectedModelIndex > 0) {
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
        static_cast<size_t>(m_state.selectedModelIndex + 1) < m_models.size()) {
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
    if (ImGui::Checkbox("Hide##Model", &hideModel)) {
      if (hideModel)
        m_modelRenderers[m_state.selectedModelIndex]->renderFlag() |=
            MODEL_RENDER_FLAG_HIDE;
      else
        m_modelRenderers[m_state.selectedModelIndex]->renderFlag() &=
            ~MODEL_RENDER_FLAG_HIDE;
    }

    bool isIKEnabled =
        m_models[m_state.selectedModelIndex]->poseSolver.isIKEnabled();
    if (ImGui::Checkbox("Enable IK", &isIKEnabled)) {
      m_models[m_state.selectedModelIndex]->poseSolver.enableIK(isIKEnabled);
    }

    const auto &motion = m_motions[m_state.selectedModelIndex];
    if (!motion->empty() && ImGui::BeginListBox("Motions")) {
      for (int i = 0; i < static_cast<int>(motion->labels().size()); ++i) {
        ImGui::PushID(i);
        if (ImGui::Selectable(motion->labels()[i].c_str(),
                              m_state.selectedMotionIndex == i))
          m_state.selectedMotionIndex = i;
        ImGui::PopID();
      }
      ImGui::EndListBox();
    }

    if (m_state.selectedMotionIndex != -1 &&
        m_state.selectedMotionIndex < static_cast<int>(motion->size())) {
      if (ImGui::Button("Remove##Motion")) {
        motion->removeMotion(m_state.selectedMotionIndex);
        m_state.selectedMotionIndex = -1;
      }
      ImGui::SameLine();
      if (ImGui::Button("Up##Motion")) {
        if (motion->moveUp(m_state.selectedMotionIndex))
          --m_state.selectedMotionIndex;
      }
      ImGui::SameLine();
      if (ImGui::Button("Down##Motion")) {
        if (motion->moveDown(m_state.selectedMotionIndex))
          ++m_state.selectedMotionIndex;
      }
    }
  }
}

void Viewer::controlPanel() {
  ImGui::Begin("Control", nullptr, ImGuiWindowFlags_NoMove);

  modelList();

  if (ImGui::TreeNode("Camera")) {
    if (ImGui::Checkbox("Ortho", &m_state.ortho))
      m_camera.projType = m_state.ortho ? glmmd::Camera::Orthographic
                                        : glmmd::Camera::Perspective;

    float fov = glm::degrees(m_camera.fov);
    if (ImGui::SliderFloat("FOV", &fov, 1.f, 120.f))
      m_camera.fov = glm::radians(fov);

    ImGui::InputFloat("Distance", &m_camera.distance);

    ImGui::InputFloat("Near", &m_camera.zNear);
    ImGui::InputFloat("Far", &m_camera.zFar);

    if (m_cameraMotion && ImGui::Button("Clear camera motion")) {
      m_cameraMotion.reset();
      initCamera();
    }
    if (ImGui::Button("Reset##Camera"))
      initCamera();

    ImGui::Checkbox("Lock", &m_state.lockCamera);

    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Physics")) {
    if (ImGui::Checkbox("Physics", &m_state.physicsEnabled)) {
      if (m_state.physicsEnabled) {
        for (auto &model : m_models)
          m_physicsWorld.setupModelPhysics(*model, true);
      } else {
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

    if (ImGui::Button("Reset")) {
      m_state.gravity = glm::vec3(0.f, -9.8f, 0.f);
      m_physicsWorld.setGravity(m_state.gravity);

      m_state.physicsSubsteps = 10;
    }

    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Render")) {

    ImGui::ColorEdit4("Clear color", &m_state.clearColor.x);

    if (ImGui::Checkbox("Render edge", &m_state.renderEdge)) {
      for (auto &renderer : m_modelRenderers)
        if (m_state.renderEdge)
          renderer->renderFlag() |= MODEL_RENDER_FLAG_EDGE;
        else
          renderer->renderFlag() &= ~MODEL_RENDER_FLAG_EDGE;
    }

    if (ImGui::Checkbox("Render ground shadow", &m_state.renderGroundShadow)) {
      for (auto &renderer : m_modelRenderers)
        if (m_state.renderGroundShadow)
          renderer->renderFlag() |= MODEL_RENDER_FLAG_GROUND_SHADOW;
        else
          renderer->renderFlag() &= ~MODEL_RENDER_FLAG_GROUND_SHADOW;
    }

    ImGui::Checkbox("Render shadow", &m_state.renderShadow);

    if (ImGui::Checkbox("Render axes", &m_state.renderAxes)) {
      m_gridRenderer->showAxes = m_state.renderAxes;
    }
    ImGui::Checkbox("Render grid", &m_state.renderGrid);

    ImGui::Checkbox("Wireframe", &m_state.wireframe);

    ImGui::SliderFloat3("Light direction", &m_mainDirectionalLight.direction.x,
                        -1.f, 1.f);

    ImGui::ColorEdit3("Light color", &m_mainDirectionalLight.color.x);

    ImGui::ColorEdit3("Ambient color", &m_mainDirectionalLight.ambientColor.x);

    if (ImGui::TreeNode("Shadow quality")) {
      auto &shadow = m_shadowMap->settings;
      ImGui::SliderFloat("Distance", &shadow.distance, 1.f, 500.f, "%.1f");
      const std::string resolution = std::to_string(m_shadowMap->width()) +
                                     " x " +
                                     std::to_string(m_shadowMap->height());
      if (ImGui::BeginCombo("Resolution", resolution.c_str())) {
        for (int size : {1024, 2048, 4096, 8192}) {
          if (ImGui::Selectable(std::to_string(size).c_str(),
                                m_shadowMap->width() == size &&
                                    m_shadowMap->height() == size))
            m_shadowMap->resize(size, size);
        }
        ImGui::EndCombo();
      }
      int filter = shadow.filterRadius - 2;
      if (ImGui::Combo("PCF filter", &filter,
                       "5 x 5\0"
                       "7 x 7\0"))
        shadow.filterRadius = filter + 2;
      ImGui::SliderFloat("Depth bias (texels)", &shadow.constantBias, 0.f, 2.f,
                         "%.2f");
      ImGui::SliderFloat("Slope bias (texels)", &shadow.slopeBias, 0.f, 3.f,
                         "%.2f");
      ImGui::SliderFloat("Normal offset (texels)", &shadow.normalBias, 0.f, 2.f,
                         "%.2f");
      ImGui::SliderFloat("Shadow alpha cutoff", &shadow.alphaCutoff, 0.f, 1.f,
                         "%.2f");
      ImGui::TreePop();
    }

    ImGui::TreePop();
  }

  ImGui::End();
}

void Viewer::profiler() {
  ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoMove);

  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("Physics: %.3f ms", m_profiler.averageTime("Physics"));
  ImGui::Text("Model update: %.3f ms", m_profiler.averageTime("Model update"));
  ImGui::Text("Render: %.3f ms", m_profiler.averageTime("Render"));
  ImGui::Text("Total: %.3f ms", m_profiler.totalTime());

  ImGui::End();
}

void Viewer::run() {
  auto &io = ImGui::GetIO();

  m_profiler.add("Physics");
  m_profiler.add("Model update");
  m_profiler.add("Render");

  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    dockspace();
    loadModelDialog();
    loadMotionDialog();

    float deltaTime = io.DeltaTime;

    m_state.progress = getProgress();

    m_profiler.startFrame();

    {
      m_profiler.start("Physics");
      const int physicsFPS[3]{60, 120, 240};
      m_physicsWorld.update(deltaTime, m_state.physicsSubsteps,
                            1.f / physicsFPS[m_state.physicsFPSSelection]);
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
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

      updateViewportSize();

      if (ImGui::IsWindowFocused())
        handleInput(deltaTime);

      if (!m_state.paused && m_state.lockCamera)
        updateCameraMotion();

      m_camera.update();

      m_profiler.start("Render");
      render();
      m_profiler.stop("Render");

      ImGui::Image(m_sceneRenderer->colorTexture().id(),
                   ImVec2(static_cast<float>(m_viewportWidth),
                          static_cast<float>(m_viewportHeight)),
                   ImVec2(1, 1), ImVec2(0, 0));
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

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(m_window);
  }
}

Viewer::~Viewer() {
  m_gridRenderer.reset();

  ModelRenderer::releaseSharedToonTextures();

  m_modelRenderers.clear();
  m_sceneRenderer.reset();
  m_shadowMap.reset();

  ImGui::GetIO().Fonts->Clear();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(m_window);
  glfwTerminate();
}
