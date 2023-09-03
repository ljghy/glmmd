#ifndef GLMMD_CORE_ANIMATOR_H_
#define GLMMD_CORE_ANIMATOR_H_

#include <vector>
#include <memory>
#include <functional>

#include <glmmd/core/Motion.h>

namespace glmmd
{

struct MotionTransition
{
    int32_t fromStateIndex;
    int32_t toStateIndex;
    float   transitionTime;

    std::function<float(float)> transitionCurve = [](float t)
    { return t; }; // [0, 1] -> [0, 1], f(0) = 0, f(1) = 1
    std::function<bool(const Motion &, float)> transitionCondition =
        [](const Motion &motion, float motionTime)
    { return motionTime > motion.duration(); };
};

struct AnimatorState
{
    float motionStartTime = 0.f;
    float stateStartTime  = 0.f;

    bool    onTransition = false;
    int32_t index        = 0;
};

class Animator
{
public:
    Animator(const AnimatorState &initialState = {});

    void registerMotion(std::unique_ptr<Motion> &&motion);
    void registerTransition(const MotionTransition &transition);

    void update(float currentTime);
    void getLocalPose(float currentTime, ModelPose &pose) const;

private:
    AnimatorState m_currentState;

    std::vector<std::unique_ptr<Motion>> m_states;
    std::vector<MotionTransition>        m_transitions;
    std::vector<std::vector<int32_t>>    m_transitionTable;
};

} // namespace glmmd

#endif
