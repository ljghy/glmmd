#ifndef VIEWER_PROFILER_H_
#define VIEWER_PROFILER_H_

#include <chrono>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

template <typename Ty, size_t N>
class Profiler
{
public:
    void push(const std::string &name)
    {
        size_t i            = m_nameToIndex.size();
        m_nameToIndex[name] = i;
        m_data.resize(m_nameToIndex.size() * N);
        m_start.resize(m_nameToIndex.size());
    }

    void startFrame()
    {
        m_offset = (m_offset + 1) % N;
        m_count  = std::min(m_count + 1, N);
        for (size_t i = 0; i < m_nameToIndex.size(); ++i)
            m_data[i * N + m_offset] = Ty{};
    }

    void start(const std::string &name)
    {
        m_start[m_nameToIndex.at(name)] = std::chrono::steady_clock::now();
    }

    void stop(const std::string &name)
    {
        auto i   = m_nameToIndex.at(name);
        auto end = std::chrono::steady_clock::now();
        m_data[i * N + m_offset] =
            std::chrono::duration_cast<std::chrono::duration<Ty>>(end -
                                                                  m_start[i])
                .count();
    }

    Ty query(const std::string &name) const
    {
        auto i = m_nameToIndex.at(name);
        return std::accumulate(m_data.begin() + i * N,
                               m_data.begin() + i * N + m_count, Ty{}) /
               m_count;
    }

    Ty queryTotal() const
    {
        Ty total{};
        for (size_t i = 0; i < m_nameToIndex.size(); ++i)
            total += std::accumulate(m_data.begin() + i * N,
                                     m_data.begin() + i * N + m_count, Ty{});
        return total / m_count;
    }

private:
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> m_start;
    std::vector<Ty>                                                 m_data;
    size_t m_offset = N - 1;
    size_t m_count  = 0;

    std::unordered_map<std::string, size_t> m_nameToIndex;
};

#endif
