#include <filesystem>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VmdFileLoader.h>

namespace glmmd
{

VmdFileLoader::VmdFileLoader(const std::string &filename,
                             const ModelData &modelData, bool utf8)
    : m_modelData(modelData)
    , m_version(0)
{
    if (utf8)
    {
        std::filesystem::path path =
#if __cplusplus <= 201703L
            std::filesystem::u8path(filename);
#else
            reinterpret_cast<const char8_t *>(filename.data());
#endif

        m_fin.open(path, std::ios::binary);
    }
    else
        m_fin.open(filename, std::ios::binary);
    if (!m_fin)
        throw std::runtime_error("Failed to open file \"" + filename + "\".");
}

void VmdFileLoader::load(FixedMotionClip &clip)
{
    loadHeader();
    clip.m_frameCount = 0;
    loadBoneFrames(clip);
    loadMorphFrames(clip);
}

void VmdFileLoader::loadHeader()
{
    std::string header(30, '\0');
    m_fin.read(header.data(), 30);
    if (header == "Vocaloid Motion Data file")
        m_version = 1;
    else if (header != "Vocaloid Motion Data 0002")
        m_version = 2;
    else
        throw std::runtime_error("VMD file format error.");

    m_modelName.resize(m_version * 10);
    m_fin.read(m_modelName.data(), m_version * 10);
    m_modelName = shiftJIS_to_UTF8(m_modelName);
}

void VmdFileLoader::loadBoneFrames(FixedMotionClip &clip)
{
    uint32_t count;
    readInt(count);

    clip.m_boneFrameIndex.resize(m_modelData.bones.size());
    clip.m_boneFrames.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        char name[16];
        m_fin.read(name, 15);
        name[15] = '\0';

        FixedMotionClip::BoneKeyFrame boneFrame;

        uint32_t frameNumber;
        readInt(frameNumber);
        clip.m_frameCount = std::max(clip.m_frameCount, frameNumber);

        readFloat<3>(boneFrame.translation.x);
        glm::vec4 q;
        readFloat<4>(q.x);
        boneFrame.rotation.x = q.x;
        boneFrame.rotation.y = q.y;
        boneFrame.rotation.z = q.z;
        boneFrame.rotation.w = q.w;

        uint8_t v;
        for (int j = 0; j < 4; ++j)
        {
            readInt(v);
            boneFrame.xCurve[j] = v / 127.f;
            m_fin.seekg(3, std::ios::cur);
        }
        for (int j = 0; j < 4; ++j)
        {
            readInt(v);
            boneFrame.yCurve[j] = v / 127.f;
            m_fin.seekg(3, std::ios::cur);
        }
        for (int j = 0; j < 4; ++j)
        {
            readInt(v);
            boneFrame.zCurve[j] = v / 127.f;
            m_fin.seekg(3, std::ios::cur);
        }
        for (int j = 0; j < 4; ++j)
        {
            readInt(v);
            boneFrame.rCurve[j] = v / 127.f;
            m_fin.seekg(3, std::ios::cur);
        }

        int32_t boneIndex = m_modelData.getBoneIndex(shiftJIS_to_UTF8(name));
        if (boneIndex == -1)
            continue;

        clip.m_boneFrameIndex[boneIndex][frameNumber] =
            static_cast<uint32_t>(clip.m_boneFrames.size());
        clip.m_boneFrames.emplace_back(boneFrame);
    }
}

void VmdFileLoader::loadMorphFrames(FixedMotionClip &clip)
{
    uint32_t count;
    readInt(count);

    clip.m_morphFrameIndex.resize(m_modelData.morphs.size());
    clip.m_morphFrames.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        char name[16];
        m_fin.read(name, 15);
        name[15] = '\0';

        FixedMotionClip::MorphKeyFrame morphFrame;

        uint32_t frameNumber;
        readInt(frameNumber);
        clip.m_frameCount = std::max(clip.m_frameCount, frameNumber);

        readFloat(morphFrame.ratio);

        int32_t morphIndex = m_modelData.getMorphIndex(shiftJIS_to_UTF8(name));
        if (morphIndex == -1)
            continue;

        clip.m_morphFrameIndex[morphIndex][frameNumber] =
            static_cast<uint32_t>(clip.m_morphFrames.size());
        clip.m_morphFrames.emplace_back(morphFrame);
    }
}

} // namespace glmmd