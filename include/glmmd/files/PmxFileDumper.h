#ifndef GLMMD_FILES_PMX_FILE_DUMPER_H_
#define GLMMD_FILES_PMX_FILE_DUMPER_H_

#include <cassert>
#include <filesystem>
#include <fstream>

#include <glmmd/core/ModelData.h>
#include <glmmd/files/CodeConverter.h>

namespace glmmd
{

class PmxFileDumper
{
public:
    PmxFileDumper(const std::filesystem::path &path);
    void dump(const ModelData &data);

private:
    void dumpInfo(const ModelData &);
    void dumpVertices(const ModelData &);
    void dumpIndices(const ModelData &);
    void dumpTextures(const ModelData &);
    void dumpMaterials(const ModelData &);
    void dumpBones(const ModelData &);
    void dumpMorphs(const ModelData &);
    void dumpDisplayFrames(const ModelData &);
    void dumpRigidBodies(const ModelData &);
    void dumpJoints(const ModelData &);

    void dumpGroupMorph(const ModelData &, const Morph &);
    void dumpVertexMorph(const ModelData &, const Morph &);
    void dumpBoneMorph(const ModelData &, const Morph &);
    void dumpUVMorph(const ModelData &, const Morph &, uint8_t);
    void dumpMaterialMorph(const ModelData &, const Morph &);

    template <int count = 1>
    void writeFloat(const float &val)
    {
        m_fout.write(reinterpret_cast<const char *>(&val),
                     sizeof(float) * count);
    }

    template <typename UIntType>
    void writeUInt(const UIntType &val, int sz = sizeof(UIntType))
    {
        m_fout.write(reinterpret_cast<const char *>(&val), sz);
    }

    template <typename IntType>
    void writeInt(const IntType &val, int sz = sizeof(IntType))
    {
        if (sz == sizeof(IntType))
        {
            m_fout.write(reinterpret_cast<const char *>(&val), sz);
            return;
        }

        switch (sz)
        {
        case 1:
        {
            auto tmp = static_cast<int8_t>(val);
            m_fout.write(reinterpret_cast<const char *>(&tmp), 1);
            break;
        }
        case 2:
        {
            auto tmp = static_cast<int16_t>(val);
            m_fout.write(reinterpret_cast<const char *>(&tmp), 2);
            break;
        }
        case 4:
        {
            auto tmp = static_cast<int32_t>(val);
            m_fout.write(reinterpret_cast<const char *>(&tmp), 4);
            break;
        }
        default:
            assert(0);
            break;
        }
    }

    void writeTextBuffer(const std::string &buf)
    {
        if (m_textEncoding == EncodingMethod::UTF8)
        {
            auto sz = static_cast<uint32_t>(buf.size());
            writeUInt(sz);
            m_fout.write(buf.data(), sz);
        }
        else
        {
            auto utf16 = codeCvt<UTF8, UTF16_LE>(buf);

            auto sz = static_cast<uint32_t>(utf16.size());
            writeUInt(sz);
            m_fout.write(utf16.data(), sz);
        }
    }

private:
    std::ofstream m_fout;

    EncodingMethod m_textEncoding;
};

inline void dumpPmxFile(const std::filesystem::path &path,
                        const ModelData             &data)
{
    PmxFileDumper{path}.dump(data);
}

} // namespace glmmd

#endif
