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
#include <glmmd/render/ModelRenderer.h>

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

    void menuBar();
    void dockspace();
    void loadModelDialog();
    void loadMotionDialog();
    void loadPoseDialog();
    void updateModels();
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

    int m_shadowMapWidth  = 1024;
    int m_shadowMapHeight = 1024;

    std::vector<std::shared_ptr<glmmd::ModelData>> m_modelData;

    std::vector<glmmd::Model>         m_models;
    std::vector<glmmd::ModelRenderer> m_modelRenderers;

    std::vector<std::unique_ptr<BlendedMotion>> m_motions;

    std::unique_ptr<InfiniteGridRenderer> m_gridRenderer;

    glm::vec3       m_cameraTarget;
    glmmd::Camera   m_camera;
    glmmd::Lighting m_lighting;

    glmmd::PhysicsWorld m_physicsWorld;

    ogl::FrameBufferObject m_FBO;
    ogl::FrameBufferObject m_intermediateFBO;
    ogl::FrameBufferObject m_shadowMapFBO;

    Profiler<float, 64> m_profiler;

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

        std::string lastModelPath;
        std::string lastMotionPath;
        std::string lastPosePath;
    } m_state;
};

#endif