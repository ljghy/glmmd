#include <glmmd/files/VmdFileLoader.h>

#include "BinaryReader.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace glmmd {

class VmdFileLoader {
public:
  VmdFileLoader() = default;
  VmdFileLoader(const VmdFileLoader &) = delete;
  VmdFileLoader(VmdFileLoader &&) = delete;
  VmdFileLoader &operator=(const VmdFileLoader &) = delete;
  VmdFileLoader &operator=(VmdFileLoader &&) = delete;

  VmdData load(const std::filesystem::path &path);

private:
  void loadHeader(VmdData &data);
  void loadBoneFrames(VmdData &data);
  void loadMorphFrames(VmdData &data);
  void loadCameraFrames(VmdData &data);

  // Reads a frame count and validates it against the remaining file size.
  uint32_t readFrameCount(std::uintmax_t frameSize) {
    uint32_t count;
    m_reader.read(count);
    m_reader.ensureAvailable(count, frameSize);
    return count;
  }

  BinaryReader m_reader;
};

VmdData VmdFileLoader::load(const std::filesystem::path &path) {
  m_reader.open(path);

  VmdData data;

  loadHeader(data);
  loadBoneFrames(data);
  loadMorphFrames(data);
  loadCameraFrames(data);

  return data;
}

void VmdFileLoader::loadHeader(VmdData &data) {
  char header[31];
  m_reader.readBytes(header, 30);
  header[30] = '\0';

  if (strcmp(header, "Vocaloid Motion Data file") == 0)
    data.version = 1;
  else if (strcmp(header, "Vocaloid Motion Data 0002") == 0)
    data.version = 2;
  else
    throw std::runtime_error("VMD file format error.");

  char modelName[21];
  m_reader.readBytes(modelName, data.version * 10);
  modelName[data.version * 10] = '\0';

  data.modelName = modelName;
}

void VmdFileLoader::loadBoneFrames(VmdData &data) {
  constexpr std::uintmax_t frameSize = 15 + 4 + 12 + 16 + 64;
  uint32_t count = readFrameCount(frameSize);

  data.boneFrames.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    auto &boneFrame = data.boneFrames[i];

    char name[16];
    m_reader.readBytes(name, 15);
    name[15] = '\0';

    boneFrame.boneName = name;

    m_reader.read(boneFrame.frameNumber);

    m_reader.readFloat<3>(boneFrame.translation.x);

    glm::vec4 q;
    m_reader.readFloat<4>(q.x);
    boneFrame.rotation.x = q.x;
    boneFrame.rotation.y = q.y;
    boneFrame.rotation.z = q.z;
    boneFrame.rotation.w = q.w;

    m_reader.readBytes(boneFrame.interpolation, 64);
  }
}

void VmdFileLoader::loadMorphFrames(VmdData &data) {
  constexpr std::uintmax_t frameSize = 15 + 4 + 4;
  uint32_t count = readFrameCount(frameSize);

  data.morphFrames.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    auto &morphFrame = data.morphFrames[i];

    char name[16];
    m_reader.readBytes(name, 15);
    name[15] = '\0';

    morphFrame.morphName = name;

    m_reader.read(morphFrame.frameNumber);
    m_reader.readFloat(morphFrame.weight);
  }
}

void VmdFileLoader::loadCameraFrames(VmdData &data) {
  constexpr std::uintmax_t frameSize = 4 + 4 + 12 + 12 + 24 + 4 + 1;

  // Camera frames are optional; older/plain motion files may end here.
  if (m_reader.bytesRemaining() < sizeof(uint32_t))
    return;

  uint32_t count = readFrameCount(frameSize);

  data.cameraFrames.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    auto &cameraFrame = data.cameraFrames[i];

    m_reader.read(cameraFrame.frameNumber);
    m_reader.readFloat(cameraFrame.distance);
    m_reader.readFloat<3>(cameraFrame.target.x);
    m_reader.readFloat<3>(cameraFrame.rotation.x);
    m_reader.readBytes(cameraFrame.interpolation, 24);
    m_reader.read(cameraFrame.fov);
    m_reader.read(cameraFrame.perspective);
  }
}

VmdData loadVmdFile(const std::filesystem::path &path) {
  return VmdFileLoader{}.load(path);
}

} // namespace glmmd
