#ifndef GLMMD_CORE_MODEL_H_
#define GLMMD_CORE_MODEL_H_

#include <memory>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/Animator.h>
#include <glmmd/core/ModelPose.h>
#include <glmmd/core/ModelPoseSolver.h>

namespace glmmd
{

class Model
{
public:
    Model(std::unique_ptr<ModelData> &&data)
        : m_data(std::move(data))
        , m_physics{}
        , m_pose(*m_data)
        , m_poseSolver(*m_data)
    {
    }

    ModelData       &data() { return *m_data; }
    const ModelData &data() const { return *m_data; }

    ModelPose       &pose() { return m_pose; }
    const ModelPose &pose() const { return m_pose; }

    ModelPhysics       &physics() { return m_physics; }
    const ModelPhysics &physics() const { return m_physics; }

    void addAnimator(std::unique_ptr<Animator> &&animator)
    {
        m_animators.emplace_back(std::move(animator));
    }

    void update(float currentTime);

    void resetLocalPose();
    void updateAnimators(float currentTime);

private:
    std::unique_ptr<ModelData>             m_data;
    ModelPhysics                           m_physics;
    ModelPose                              m_pose;
    ModelPoseSolver                        m_poseSolver;
    std::vector<std::unique_ptr<Animator>> m_animators;
};

} // namespace glmmd

#endif