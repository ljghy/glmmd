#ifndef GLMMD_FILES_PMX_FILE_DUMPER_H_
#define GLMMD_FILES_PMX_FILE_DUMPER_H_

#include <fstream>
#include <filesystem>
#include <cassert>

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

    void dumpGroupMorph(const ModelData &, const GroupMorph &);
    void dumpVertexMorph(const ModelData &, const VertexMorph &);
    void dumpBoneMorph(const ModelData &, const BoneMorph &);
    void dumpUVMorph(const ModelData &, const UVMorph &, uint8_t);
    void dumpMaterialMorph(const ModelData &, const MaterialMorph &);

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
            int8_t tmp = static_cast<int8_t>(val);
            m_fout.write(reinterpret_cast<const char *>(&tmp), 1);
            break;
        }
        case 2:
        {
            int16_t tmp = static_cast<int16_t>(val);
            m_fout.write(reinterpret_cast<const char *>(&tmp), 2);
            break;
        }
        case 4:
        {
            int32_t tmp = static_cast<int32_t>(val);
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
        if (m_textEncoding == m_internalTextEncoding)
        {
            uint32_t sz = static_cast<uint32_t>(buf.size());
            writeUInt(sz);
            m_fout.write(buf.data(), sz);
        }
        else if (m_textEncoding == EncodingMethod::UTF16_LE)
        // internal is UTF-8, convert to UTF-16 LE
        {
            auto utf16 = codeCvt<UTF8, UTF16_LE>(buf);

            uint32_t sz = static_cast<uint32_t>(utf16.size());
            writeUInt(sz);
            m_fout.write(utf16.data(), sz);
        }
        else
        // internal is UTF-16 LE, convert to UTF-8
        {
            auto utf8 = codeCvt<UTF16_LE, UTF8>(buf);

            uint32_t sz = static_cast<uint32_t>(utf8.size());
            writeUInt(sz);
            m_fout.write(utf8.data(), sz);
        }
    }

private:
    std::ofstream m_fout;

    EncodingMethod m_textEncoding;
    EncodingMethod m_internalTextEncoding;
};

inline void dumpPmxFile(const std::filesystem::path &path,
                        const ModelData             &data)
{
    PmxFileDumper{path}.dump(data);
}

} // namespace glmmd

#endif
