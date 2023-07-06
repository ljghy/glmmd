#ifndef GLMMD_FILES_VMD_FILE_LOADER_H_
#define GLMMD_FILES_VMD_FILE_LOADER_H_

#include <fstream>

#include <glmmd/core/ModelData.h>
#include <glmmd/core/FixedMotionClip.h>

namespace glmmd
{

class VmdFileLoader
{
public:
    VmdFileLoader(const std::string &filename, const ModelData &modelData,
                  bool utf8 = false);
    void load(FixedMotionClip &clip);

    int                version() const { return m_version; }
    const std::string &modelName() const { return m_modelName; }

private:
    void loadHeader();
    void loadBoneFrames(FixedMotionClip &clip);
    void loadMorphFrames(FixedMotionClip &clip);

    template <int count = 1>
    void readFloat(float &val)
    {
        for (int i = 0; i < count; ++i)
            m_fin.read(reinterpret_cast<char *>(&val + i), sizeof(float));
    }

    template <typename IntType>
    void readInt(IntType &val, int sz = sizeof(IntType))
    {
        m_fin.read(reinterpret_cast<char *>(&val), sz);
    }

private:
    std::ifstream    m_fin;
    const ModelData &m_modelData;

    int         m_version;
    std::string m_modelName;
};

} // namespace glmmd

#endif