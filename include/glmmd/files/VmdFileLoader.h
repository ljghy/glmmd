#ifndef GLMMD_FILES_VMD_FILE_LOADER_H_
#define GLMMD_FILES_VMD_FILE_LOADER_H_

#include <fstream>
#include <filesystem>

#include <glmmd/files/VmdData.h>

namespace glmmd
{

class VmdFileLoader
{
public:
    VmdFileLoader()                                 = default;
    VmdFileLoader(const VmdFileLoader &)            = delete;
    VmdFileLoader(VmdFileLoader &&)                 = delete;
    VmdFileLoader &operator=(const VmdFileLoader &) = delete;
    VmdFileLoader &operator=(VmdFileLoader &&)      = delete;

    std::shared_ptr<VmdData> load(const std::filesystem::path &path);

private:
    void loadHeader(VmdData &data);
    void loadBoneFrames(VmdData &data);
    void loadMorphFrames(VmdData &data);
    void loadCameraFrames(VmdData &data);

    template <int count = 1>
    void readFloat(float &val)
    {
        for (int i = 0; i < count; ++i)
            m_fin.read(reinterpret_cast<char *>(&val + i), sizeof(float));
    }

    template <typename UIntType>
    void readUInt(UIntType &val, int sz = sizeof(UIntType))
    {
        m_fin.read(reinterpret_cast<char *>(&val), sz);
    }

private:
    std::ifstream m_fin;
};

inline std::shared_ptr<VmdData> loadVmdFile(const std::filesystem::path &path)
{
    return VmdFileLoader{}.load(path);
}

} // namespace glmmd

#endif