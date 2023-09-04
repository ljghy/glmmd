#ifndef GLMMD_CORE_ANIMATOR_H_
#define GLMMD_CORE_ANIMATOR_H_

#include <chrono>

#include <glmmd/core/Motion.h>

namespace glmmd
{

class Animator
{
public:
    Animator()
        : m_startTime(std::chrono::high_resolution_clock::now())
    {
    }

    virtual ~Animator() = default;

    void  reset() { m_startTime = std::chrono::high_resolution_clock::now(); }
    float getElapsedTime() const
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(now - m_startTime).count();
    }

    virtual bool isFinished() const                  = 0;
    virtual void getLocalPose(ModelPose &pose) const = 0;

protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};

} // namespace glmmd

#endif
