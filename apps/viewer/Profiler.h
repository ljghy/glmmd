#ifndef VIEWER_PROFILER_H_
#define VIEWER_PROFILER_H_

#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

template <size_t WindowSize = 64>
class Profiler
{
public:
    Profiler() { std::fill(m_frameTime, m_frameTime + WindowSize, 0.f); }

    void startFrame()
    {
        m_totalTime -= m_frameTime[m_offset];
        m_frameTime[m_offset] = 0.f;
    }

    void start() { m_start = std::chrono::steady_clock::now(); }

    void stop()
    {
        auto end = std::chrono::steady_clock::now();
        m_frameTime[m_offset] +=
            std::chrono::duration<float, std::milli>(end - m_start).count();
    }

    void endFrame()
    {
        if (m_count < WindowSize)
            ++m_count;

        m_totalTime += m_frameTime[m_offset];
        m_averageTime = m_totalTime / m_count;

        ++m_offset;
        if (m_offset == WindowSize)
            m_offset = 0;
    }

    float averageTime() const { return m_averageTime; }

private:
    size_t m_offset{};
    size_t m_count{};

    std::chrono::time_point<std::chrono::steady_clock> m_start;

    float m_frameTime[WindowSize];

    float m_totalTime{};
    float m_averageTime{};
};

template <size_t WindowSize = 64>
class ProfilerSet
{
public:
    void add(const std::string &name)
    {
        auto [_, inserted] = m_indices.emplace(name, m_profilers.size());
        if (inserted)
            m_profilers.emplace_back();
    }

    void startFrame()
    {
        for (auto &profiler : m_profilers)
            profiler.startFrame();
    }

    void start(const std::string &name)
    {
        m_profilers[m_indices.at(name)].start();
    }

    void stop(const std::string &name)
    {
        m_profilers[m_indices.at(name)].stop();
    }

    void endFrame()
    {
        for (auto &profiler : m_profilers)
            profiler.endFrame();
    }

    float averageTime(const std::string &name) const
    {
        return m_profilers[m_indices.at(name)].averageTime();
    }

    float totalTime() const
    {
        float total = 0.f;
        for (const auto &profiler : m_profilers)
            total += profiler.averageTime();
        return total;
    }

private:
    std::unordered_map<std::string, size_t> m_indices;
    std::vector<Profiler<WindowSize>>       m_profilers;
};

#endif
