#ifndef VIEWER_H_
#define VIEWER_H_

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <memory>
#include <chrono>
#include <filesystem>

#include <glmmd/core/Model.h>
#include <glmmd/core/PhysicsWorld.h>
#include <glmmd/core/CameraMotion.h>

#include "ModelRenderer.h"
#include "JsonParser.hpp"
#include "BlendedMotion.h"
#include "InfiniteGridRenderer.h"
#include "Profiler.h"

class Viewer
{
    friend void dropCallback(GLFWwindow *window, int count, const char **paths);

public:
    Viewer(const std::filesystem::path &executableDir);
    ~Viewer();

    void run();

private:
    void initWindow();
    void initImGui();
    void initFBO();
    void loadResources();

    bool loadModel(const std::filesystem::path &path);
    void removeModel(size_t i);

    void loadMotion(const std::filesystem::path &path, size_t modelIndex,
                    const JsonNode &config = JsonObj_t{});
    void loadPose(const std::filesystem::path &path, size_t modelIndex);

    void updateModelPose(size_t i);

    void handleInput(float deltaTime);

    void initState();

    void initCamera();
    void initMainLight();

    void menuBar();
    void dockspace();
    void loadModelDialog();
    void loadMotionDialog();
    void loadPoseDialog();
    void updateModels();
    void updateCameraMotion();
    void updateViewportSize();
    void render();
    void progress();

    void modelList();
    void controlPanel();
    void profiler();

    void  play();
    void  pause();
    void  resetProgress();
    float getProgress() const;
    void  setProgress(float progress);

private:
    std::filesystem::path m_executableDir;
    JsonNode              m_initData;

    float m_uiScale;

    GLFWwindow *m_window;

    int m_viewportWidth;
    int m_viewportHeight;

    int m_shadowMapWidth;
    int m_shadowMapHeight;

    std::vector<std::unique_ptr<glmmd::Model>>  m_models;
    std::vector<std::unique_ptr<ModelRenderer>> m_modelRenderers;
    std::vector<std::unique_ptr<BlendedMotion>> m_motions;

    std::unique_ptr<glmmd::CameraMotion> m_cameraMotion;

    std::unique_ptr<InfiniteGridRenderer> m_gridRenderer;

    glmmd::Camera           m_camera;
    glmmd::DirectionalLight m_mainDirectionalLight;

    glmmd::PhysicsWorld m_physicsWorld;

    ogl::FrameBufferObject m_FBO;
    ogl::FrameBufferObject m_intermediateFBO;
    ogl::FrameBufferObject m_shadowMapFBO;

    ProfilerSet<> m_profiler;

    struct State
    {
        bool showControlPanel;
        bool showProfiler;
        bool showProgress;

        int selectedModelIndex;
        int selectedMotionIndex;

        bool                                               paused;
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        std::chrono::time_point<std::chrono::steady_clock> pauseTime;
        float                                              progress;

        bool physicsEnabled;
        int  physicsFPSSelection;

        glm::vec3 gravity;

        glm::vec4 clearColor;

        bool ortho;
        bool renderEdge;
        bool renderShadow;
        bool renderGroundShadow;
        bool renderAxes;
        bool renderGrid;
        bool wireframe;
        bool lockCamera;

        float shadowDistance;

        std::string lastModelPath;
        std::string lastMotionPath;
        std::string lastPosePath;
    } m_state;
};

#endif