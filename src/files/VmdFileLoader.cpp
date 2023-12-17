#include <filesystem>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VmdFileLoader.h>

namespace glmmd
{

std::shared_ptr<VmdData> VmdFileLoader::load(const std::string &filename,
                                             bool               utf8Path)
{
    if (utf8Path)
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

    auto data = std::make_shared<VmdData>();

    loadHeader(*data);
    loadBoneFrames(*data);
    loadMorphFrames(*data);
    loadCameraFrames(*data);

    return data;
}

void VmdFileLoader::loadHeader(VmdData &data)
{
    std::string header(30, '\0');
    m_fin.read(header.data(), 30);
    if (header == "Vocaloid Motion Data file")
        data.version = 1;
    else if (header != "Vocaloid Motion Data 0002")
        data.version = 2;
    else
        throw std::runtime_error("VMD file format error.");

    char modelName[21];
    m_fin.read(modelName, data.version * 10);
    modelName[data.version * 10] = '\0';

    data.modelName = modelName;
}

void VmdFileLoader::loadBoneFrames(VmdData &data)
{
    uint32_t count;
    readUInt(count);

    data.boneFrames.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        auto &boneFrame = data.boneFrames[i];

        char name[16];
        m_fin.read(name, 15);
        name[15]           = '\0';
        boneFrame.boneName = name;

        readUInt(boneFrame.frameNumber);

        readFloat<3>(boneFrame.translation.x);

        glm::vec4 q;
        readFloat<4>(q.x);
        boneFrame.rotation.x = q.x;
        boneFrame.rotation.y = q.y;
        boneFrame.rotation.z = q.z;
        boneFrame.rotation.w = q.w;

        m_fin.read(reinterpret_cast<char *>(boneFrame.interpolation), 64);
    }
}

void VmdFileLoader::loadMorphFrames(VmdData &data)
{
    uint32_t count;
    readUInt(count);

    data.morphFrames.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        auto &morphFrame = data.morphFrames[i];

        char name[16];
        m_fin.read(name, 15);
        name[15]             = '\0';
        morphFrame.morphName = name;
        readUInt(morphFrame.frameNumber);
        readFloat(morphFrame.ratio);
    }
}

void VmdFileLoader::loadCameraFrames(VmdData &data)
{
    uint32_t count;
    readUInt(count);

    data.cameraFrames.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        auto &cameraFrame = data.cameraFrames[i];

        readUInt(cameraFrame.frameNumber);
        readFloat(cameraFrame.distance);
        readFloat<3>(cameraFrame.targetPosition.x);
        readFloat<3>(cameraFrame.rotation.x);
        m_fin.read(reinterpret_cast<char *>(cameraFrame.interpolation), 24);
        readUInt(cameraFrame.fov);
        readUInt(cameraFrame.perspective);
    }
}

} // namespace glmmd