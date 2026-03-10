#ifndef GLMMD_CORE_MODEL_H_
#define GLMMD_CORE_MODEL_H_

#include <memory>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/ModelPhysics.h>
#include <glmmd/core/ModelPose.h>
#include <glmmd/core/ModelPoseSolver.h>

namespace glmmd
{

struct Model
{
    Model(const std::shared_ptr<ModelData> &data)
        : data(data)
        , pose(data)
        , poseSolver(data)
    {
        pose.resetLocal();
        poseSolver.solveBeforePhysics(pose);
        poseSolver.solveAfterPhysics(pose);
    }

    Model(const Model &other)            = delete;
    Model &operator=(const Model &other) = delete;

    Model(Model &&other) noexcept            = default;
    Model &operator=(Model &&other) noexcept = default;

    void solvePose()
    {
        poseSolver.solveBeforePhysics(pose);
        poseSolver.syncWithPhysics(pose, physics);
        poseSolver.solveAfterPhysics(pose);
    }

    std::shared_ptr<ModelData> data;
    ModelPhysics               physics;
    ModelPose                  pose;
    ModelPoseSolver            poseSolver;
};

} // namespace glmmd

#endif