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
        reset();
    }

    virtual ~Animator() = default;

    void pause()
    {
        m_paused    = true;
        m_pauseTime = std::chrono::high_resolution_clock::now();
    }
    void resume()
    {
        m_paused = false;
        auto now = std::chrono::high_resolution_clock::now();
        m_startTime += now - m_pauseTime;
    }
    bool isPaused() const { return m_paused; }

    void reset()
    {
        m_startTime = std::chrono::high_resolution_clock::now();
        if (m_paused)
            m_pauseTime = m_startTime;
    }

    float getElapsedTime() const

    {
        if (m_paused)
            return std::chrono::duration<float>(m_pauseTime - m_startTime)
                .count();
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(now - m_startTime).count();
    }

    virtual bool isFinished() const                  = 0;
    virtual void getLocalPose(ModelPose &pose) const = 0;

protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_pauseTime;
    bool                                                        m_paused = true;
};

} // namespace glmmd

#endif
