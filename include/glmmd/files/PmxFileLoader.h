#ifndef GLMMD_FILES_PMX_FILE_LOADER_H_
#define GLMMD_FILES_PMX_FILE_LOADER_H_

#include <fstream>
#include <filesystem>

#include <glmmd/core/ModelData.h>

namespace glmmd
{

class PmxFileLoader
{
public:
    PmxFileLoader(const std::string &filename, bool utf8 = false);
    void load(ModelData &data);

private:
    void loadInfo(ModelData &);
    void loadVertices(ModelData &);
    void loadIndices(ModelData &);
    void loadTextures(ModelData &);
    void loadMaterials(ModelData &);
    void loadBones(ModelData &);
    void loadMorphs(ModelData &);
    void loadDisplayFrames(ModelData &);
    void loadRigidBodies(ModelData &);
    void loadJoints(ModelData &);

    Morph::MorphData loadGroupMorph(ModelData &);
    Morph::MorphData loadVertexMorph(ModelData &);
    Morph::MorphData loadBoneMorph(ModelData &);
    Morph::MorphData loadUVMorph(ModelData &, uint8_t);
    Morph::MorphData loadMaterialMorph(ModelData &);

    template <int count = 1>
    void readFloat(float &val)
    {
        for (int i = 0; i < count; ++i)
            m_fin.read(reinterpret_cast<char *>(&val + i), sizeof(float));
    }

    template <typename UIntType>
    void readUInt(UIntType &val)
    {
        m_fin.read(reinterpret_cast<char *>(&val), sizeof(UIntType));
    }

    template <typename UIntType>
    void readUInt(UIntType &val, int sz)
    {
        switch (sz)
        {
        case 1:
        {
            uint8_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<UIntType>(tmp);
            break;
        }
        case 2:
        {
            uint16_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<UIntType>(tmp);
            break;
        }
        case 4:
        {
            uint32_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<UIntType>(tmp);
            break;
        }
        default:
            m_fin.read(reinterpret_cast<char *>(&val), sz);
        }
    }

    template <typename IntType>
    void readInt(IntType &val)
    {
        m_fin.read(reinterpret_cast<char *>(&val), sizeof(IntType));
    }

    template <typename IntType>
    void readInt(IntType &val, int sz)
    {
        switch (sz)
        {
        case 1:
        {
            int8_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<IntType>(tmp);
            break;
        }
        case 2:
        {
            int16_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<IntType>(tmp);
            break;
        }
        case 4:
        {
            int32_t tmp;
            m_fin.read(reinterpret_cast<char *>(&tmp), sz);
            val = static_cast<IntType>(tmp);
            break;
        }
        default:
            m_fin.read(reinterpret_cast<char *>(&val), sz);
        }
    }

    void readTextBuffer(std::string &buf)
    {
        uint32_t sz;
        readUInt(sz);
        buf.resize(sz);
        m_fin.read(buf.data(), sz);
    }

private:
    std::ifstream         m_fin;
    std::filesystem::path m_modelDir;
};

} // namespace glmmd

#endif