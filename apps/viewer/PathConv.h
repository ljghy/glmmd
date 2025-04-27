#ifndef VIEWER_PATH_CONV_H_
#define VIEWER_PATH_CONV_H_

#include <filesystem>
#include <string>

inline std::filesystem::path u8stringToPath(const std::string &u8path)
{
    return std::u8string(u8path.begin(), u8path.end());
}

inline std::string pathToU8string(const std::filesystem::path &path)
{
    auto u8path = path.u8string();
    return {u8path.begin(), u8path.end()};
}

#endif
