#ifndef GLMMD_FILES_VPD_FILE_LOADER_H_
#define GLMMD_FILES_VPD_FILE_LOADER_H_

#include <filesystem>
#include <memory>

#include <glmmd/files/VpdData.h>

namespace glmmd
{

class VpdFileLoader
{
public:
    VpdFileLoader()                                 = default;
    VpdFileLoader(const VpdFileLoader &)            = delete;
    VpdFileLoader(VpdFileLoader &&)                 = delete;
    VpdFileLoader &operator=(const VpdFileLoader &) = delete;
    VpdFileLoader &operator=(VpdFileLoader &&)      = delete;

    std::shared_ptr<VpdData> load(const std::filesystem::path &path);

private:
    void removeComment();

    void readLine(std::string &line);
    void skipSpace();
    bool readUntil(char end);
    bool readUntil(std::string &buffer, char end);
    bool readUntilSpace(std::string &buffer);
    bool s2i(const std::string &str, int &value);
    bool s2f(const std::string &str, float &value);

    void loadHeader(VpdData &data);
    void loadBoneData(VpdData &data);

private:
    size_t      m_pos;
    std::string m_buffer;
};

inline std::shared_ptr<VpdData> loadVpdFile(const std::filesystem::path &path)
{
    return VpdFileLoader{}.load(path);
}

} // namespace glmmd

#endif
