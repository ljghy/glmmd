#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>

#include "BinaryReader.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace glmmd {

class PmxFileLoader {
public:
  PmxFileLoader() = default;
  PmxFileLoader(const PmxFileLoader &) = delete;
  PmxFileLoader(PmxFileLoader &&) = delete;
  PmxFileLoader &operator=(const PmxFileLoader &) = delete;
  PmxFileLoader &operator=(PmxFileLoader &&) = delete;

  ModelData load(const std::filesystem::path &path);

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

  void loadGroupMorph(const ModelData &, Morph &);
  void loadVertexMorph(const ModelData &, Morph &);
  void loadBoneMorph(const ModelData &, Morph &);
  void loadUVMorph(const ModelData &, Morph &, uint8_t);
  void loadMaterialMorph(const ModelData &, Morph &);

  void validate(const ModelData &) const;

  template <int count = 1> void readFloat(float &val) {
    m_reader.readFloat<count>(val);
  }

  template <typename UIntType> void readUInt(UIntType &val) {
    m_reader.read(val);
  }

  template <typename UIntType> void readUInt(UIntType &val, int sz) {
    m_reader.readUInt(val, sz);
  }

  template <typename IntType> void readInt(IntType &val) { m_reader.read(val); }

  template <typename IntType> void readInt(IntType &val, int sz) {
    m_reader.readInt(val, sz);
  }

  void readTextBuffer(std::string &buf) {
    uint32_t sz;
    readUInt(sz);
    m_reader.ensureAvailable(sz);
    buf.resize(sz);
    m_reader.readBytes(buf.data(), sz);
  }

  int32_t readCount() {
    int32_t count;
    readInt(count);
    if (count < 0)
      throw std::runtime_error("PMX file contains a negative element count.");
    m_reader.ensureAvailable(static_cast<std::uintmax_t>(count));
    return count;
  }

private:
  BinaryReader m_reader;
};

ModelData PmxFileLoader::load(const std::filesystem::path &path) {
  m_reader.open(path);

  ModelData data;

  data.baseDir = path.parent_path();

  loadInfo(data);
  loadVertices(data);
  loadIndices(data);
  loadTextures(data);
  loadMaterials(data);
  loadBones(data);
  loadMorphs(data);
  loadDisplayFrames(data);
  loadRigidBodies(data);
  loadJoints(data);

  validate(data);

  return data;
}

void PmxFileLoader::loadInfo(ModelData &data) {
  ModelInfo &info = data.info;

  char header[4];
  m_reader.readBytes(header, 4);
  if (header[0] != 'P' || header[1] != 'M' || header[2] != 'X' ||
      header[3] != ' ') // "PMX "
    throw std::runtime_error("PMX file format error.");

  readFloat(info.version);
  if (info.version != 2.0f && info.version != 2.1f)
    throw std::runtime_error("PMX file format error.");

  uint8_t byteSize;
  readUInt(byteSize);
  if (byteSize != 8)
    throw std::runtime_error("PMX file format error.");

  readUInt(info.encoding);
  readUInt(info.additionalUVNum);
  readUInt(info.vertexIndexSize);
  readUInt(info.textureIndexSize);
  readUInt(info.materialIndexSize);
  readUInt(info.boneIndexSize);
  readUInt(info.morphIndexSize);
  readUInt(info.rigidBodyIndexSize);

  readTextBuffer(info.modelName);
  readTextBuffer(info.modelNameEN);
  readTextBuffer(info.comment);
  readTextBuffer(info.commentEN);

  if (info.encoding == Encoding::UTF16_LE) {
    info.modelName = codeCvt<UTF16_LE, UTF8>(info.modelName);
    info.modelNameEN = codeCvt<UTF16_LE, UTF8>(info.modelNameEN);
    info.comment = codeCvt<UTF16_LE, UTF8>(info.comment);
    info.commentEN = codeCvt<UTF16_LE, UTF8>(info.commentEN);
  }
}

void PmxFileLoader::loadVertices(ModelData &data) {
  int32_t count = readCount();
  data.vertices.resize(count);

  if (data.info.additionalUVNum > 0)
    data.additionalUVs.resize(count);

  int32_t index = 0;

  for (auto &vert : data.vertices) {
    readFloat<3>(vert.position.x);
    readFloat<3>(vert.normal.x);
    readFloat<2>(vert.uv.x);

    for (int i = 0; i < data.info.additionalUVNum; ++i)
      readFloat<4>(data.additionalUVs[index][i].x);
    ++index;

    readUInt(vert.skinningType);

    auto &sz = data.info.boneIndexSize;
    switch (vert.skinningType) {
    case SkinningType::BDEF1:
      readInt(vert.boneIndices[0], sz);
      break;
    case SkinningType::BDEF2:
      readInt(vert.boneIndices[0], sz);
      readInt(vert.boneIndices[1], sz);
      readFloat<1>(vert.boneWeights[0]);
      vert.boneWeights[1] = 1.f - vert.boneWeights[0];
      break;
    case SkinningType::BDEF4:
      readInt(vert.boneIndices[0], sz);
      readInt(vert.boneIndices[1], sz);
      readInt(vert.boneIndices[2], sz);
      readInt(vert.boneIndices[3], sz);
      readFloat<4>(vert.boneWeights[0]);
      break;
    case SkinningType::SDEF:
      readInt(vert.boneIndices[0], sz);
      readInt(vert.boneIndices[1], sz);
      readFloat<1>(vert.boneWeights[0]);
      readFloat<3>(vert.sdefC.x);
      readFloat<3>(vert.sdefR0.x);
      readFloat<3>(vert.sdefR1.x);
      break;
    case SkinningType::QDEF:
      readInt(vert.boneIndices[0], sz);
      readInt(vert.boneIndices[1], sz);
      readInt(vert.boneIndices[2], sz);
      readInt(vert.boneIndices[3], sz);
      readFloat<4>(vert.boneWeights[0]);
      break;
    }
    readFloat(vert.edgeScale);
  }
}

void PmxFileLoader::loadIndices(ModelData &data) {
  int32_t count = readCount();
  data.indices.resize(count);

  for (auto &index : data.indices)
    readUInt(index, data.info.vertexIndexSize);
}

void PmxFileLoader::loadTextures(ModelData &data) {
  int32_t count = readCount();
  data.texturePaths.resize(count);

  for (auto &path : data.texturePaths) {
    readTextBuffer(path);
    if (data.info.encoding == Encoding::UTF16_LE)
      path = codeCvt<UTF16_LE, UTF8>(path);
#ifndef _WIN32
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
  }
}

void PmxFileLoader::loadMaterials(ModelData &data) {
  int32_t count = readCount();
  data.materials.resize(count);

  for (auto &mat : data.materials) {
    readTextBuffer(mat.name);
    readTextBuffer(mat.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      mat.name = codeCvt<UTF16_LE, UTF8>(mat.name);
      mat.nameEN = codeCvt<UTF16_LE, UTF8>(mat.nameEN);
    }

    readFloat<4>(mat.diffuse.x);
    readFloat<3>(mat.specular.x);
    readFloat(mat.specularPower);
    readFloat<3>(mat.ambient.x);
    readUInt(mat.flags);

    readFloat<4>(mat.edgeColor.x);
    readFloat(mat.edgeSize);
    readInt(mat.textureIndex, data.info.textureIndexSize);
    readInt(mat.sphereTextureIndex, data.info.textureIndexSize);
    readUInt(mat.sphereMode);
    readUInt(mat.sharedToonFlag);
    if (mat.sharedToonFlag == 0)
      readInt(mat.toonTextureIndex, data.info.textureIndexSize);
    else
      readInt(mat.toonTextureIndex, 1);
    readTextBuffer(mat.memo);
    if (data.info.encoding == Encoding::UTF16_LE)
      mat.memo = codeCvt<UTF16_LE, UTF8>(mat.memo);
    readInt(mat.indicesCount);
  }
}

void PmxFileLoader::loadBones(ModelData &data) {
  int32_t count = readCount();
  data.bones.resize(count);

  int32_t i = 0;
  for (auto &bone : data.bones) {
    readTextBuffer(bone.name);
    readTextBuffer(bone.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      bone.name = codeCvt<UTF16_LE, UTF8>(bone.name);
      bone.nameEN = codeCvt<UTF16_LE, UTF8>(bone.nameEN);
    }
    readFloat<3>(bone.position.x);
    readInt(bone.parentIndex, data.info.boneIndexSize);
    readInt(bone.deformLayer);

    readUInt(bone.flags);

    if (bone.endByBoneIndex())
      readInt(bone.endIndex, data.info.boneIndexSize);
    else
      readFloat<3>(bone.endPosition.x);

    if (bone.inheritRotation() || bone.inheritTranslation()) {
      readInt(bone.inheritParentIndex, data.info.boneIndexSize);
      readFloat(bone.inheritWeight);
    }

    if (bone.limitAxis())
      readFloat<3>(bone.axisDirection.x);

    if (bone.localAxis()) {
      readFloat<3>(bone.localXVector.x);
      readFloat<3>(bone.localZVector.x);
    }

    if (bone.externalParent())
      readInt(bone.externalParentKey);

    if (bone.isIK()) {
      bone.ikDataIndex = static_cast<int32_t>(data.ikData.size());
      data.ikData.emplace_back();
      auto &ik = data.ikData.back();

      ik.targetBoneIndex = i;
      readInt(ik.endEffector, data.info.boneIndexSize);
      readInt(ik.loopCount);
      readFloat(ik.limitAngle);

      int32_t ikLinkCount = readCount();
      ik.links.resize(ikLinkCount);
      for (auto &link : ik.links) {
        readInt(link.boneIndex, data.info.boneIndexSize);
        readUInt(link.angleLimitFlag);
        if (link.angleLimitFlag) {
          readFloat<3>(link.lowerLimit.x);
          readFloat<3>(link.upperLimit.x);
        }
      }
    }
    ++i;
  }
}

void PmxFileLoader::loadMorphs(ModelData &data) {
  int32_t count = readCount();
  data.morphs.resize(count);

  for (auto &morph : data.morphs) {
    readTextBuffer(morph.name);
    readTextBuffer(morph.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      morph.name = codeCvt<UTF16_LE, UTF8>(morph.name);
      morph.nameEN = codeCvt<UTF16_LE, UTF8>(morph.nameEN);
    }
    readUInt(morph.panel);
    readUInt(morph.type);
    int32_t count = readCount();
    switch (morph.type) {
    case MorphType::Group:
      morph.data.emplace<std::vector<GroupMorph>>(count);
      loadGroupMorph(data, morph);
      break;
    case MorphType::Vertex:
      morph.data.emplace<std::vector<VertexMorph>>(count);
      loadVertexMorph(data, morph);
      break;
    case MorphType::Bone:
      morph.data.emplace<std::vector<BoneMorph>>(count);
      loadBoneMorph(data, morph);
      break;
    case MorphType::UV:
    case MorphType::UV1:
    case MorphType::UV2:
    case MorphType::UV3:
    case MorphType::UV4:
      morph.data.emplace<std::vector<UVMorph>>(count);
      loadUVMorph(data, morph,
                  static_cast<uint8_t>(morph.type) -
                      static_cast<uint8_t>(MorphType::UV));
      break;
    case MorphType::Material:
      morph.data.emplace<std::vector<MaterialMorph>>(count);
      loadMaterialMorph(data, morph);
      break;
    }
  }
}

void PmxFileLoader::loadGroupMorph(const ModelData &modelData, Morph &morph) {
  for (auto &group : morph.group()) {
    readUInt(group.index, modelData.info.morphIndexSize);
    readFloat(group.weight);
  }
}

void PmxFileLoader::loadVertexMorph(const ModelData &data, Morph &morph) {
  for (auto &vertex : morph.vertex()) {
    readUInt(vertex.index, data.info.vertexIndexSize);
    readFloat<3>(vertex.offset.x);
  }
}

void PmxFileLoader::loadBoneMorph(const ModelData &data, Morph &morph) {
  for (auto &bone : morph.bone()) {
    readUInt(bone.index, data.info.boneIndexSize);
    readFloat<3>(bone.translation.x);
    glm::vec4 q;
    readFloat<4>(q.x); // internal order: x, y, z, w
    bone.rotation = glm::quat(q.w, q.x, q.y, q.z);
  }
}

void PmxFileLoader::loadUVMorph(const ModelData &data, Morph &morph,
                                uint8_t num) {
  for (auto &uv : morph.uv()) {
    for (uint8_t j = 0; j < 5; ++j)
      uv.offset[j] = glm::vec4(0.f);
    readUInt(uv.index, data.info.vertexIndexSize);
    readFloat<4>(uv.offset[num].x);
  }
}

void PmxFileLoader::loadMaterialMorph(const ModelData &data, Morph &morph) {
  for (auto &mat : morph.material()) {
    readInt(mat.index, data.info.materialIndexSize);
    readUInt(mat.operation);
    readFloat<4>(mat.diffuse.x);
    readFloat<3>(mat.specular.x);
    readFloat(mat.specularPower);
    readFloat<3>(mat.ambient.x);
    readFloat<4>(mat.edgeColor.x);
    readFloat(mat.edgeSize);
    readFloat<4>(mat.texture.x);
    readFloat<4>(mat.sphereTexture.x);
    readFloat<4>(mat.toonTexture.x);
  }
}

void PmxFileLoader::loadDisplayFrames(ModelData &data) {
  int32_t count = readCount();
  data.displayFrames.resize(count);

  for (auto &frame : data.displayFrames) {
    readTextBuffer(frame.name);
    readTextBuffer(frame.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      frame.name = codeCvt<UTF16_LE, UTF8>(frame.name);
      frame.nameEN = codeCvt<UTF16_LE, UTF8>(frame.nameEN);
    }

    readUInt(frame.specialFlag);

    int32_t elementCount = readCount();
    frame.elements.resize(elementCount);
    for (auto &element : frame.elements) {
      readUInt(element.type);
      readInt(element.index, element.type == DisplayElementType::Bone
                                 ? data.info.boneIndexSize
                                 : data.info.morphIndexSize);
    }
  }
}

void PmxFileLoader::loadRigidBodies(ModelData &data) {
  int32_t count = readCount();
  data.rigidBodies.resize(count);

  for (auto &rigidBody : data.rigidBodies) {
    readTextBuffer(rigidBody.name);
    readTextBuffer(rigidBody.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      rigidBody.name = codeCvt<UTF16_LE, UTF8>(rigidBody.name);
      rigidBody.nameEN = codeCvt<UTF16_LE, UTF8>(rigidBody.nameEN);
    }

    readInt(rigidBody.boneIndex, data.info.boneIndexSize);

    readUInt(rigidBody.group);
    readUInt(rigidBody.collisionGroupMask);
    readUInt(rigidBody.shape);
    readFloat<3>(rigidBody.size.x);
    readFloat<3>(rigidBody.position.x);
    readFloat<3>(rigidBody.rotation.x);
    readFloat(rigidBody.mass);
    readFloat(rigidBody.linearDamping);
    readFloat(rigidBody.angularDamping);
    readFloat(rigidBody.restitution);
    readFloat(rigidBody.friction);
    readUInt(rigidBody.physicsCalcType);
  }
}

void PmxFileLoader::loadJoints(ModelData &data) {
  int32_t count = readCount();
  data.joints.resize(count);

  for (auto &joint : data.joints) {
    readTextBuffer(joint.name);
    readTextBuffer(joint.nameEN);
    if (data.info.encoding == Encoding::UTF16_LE) {
      joint.name = codeCvt<UTF16_LE, UTF8>(joint.name);
      joint.nameEN = codeCvt<UTF16_LE, UTF8>(joint.nameEN);
    }

    readUInt(joint.type);
    readInt(joint.rigidBodyIndexA, data.info.rigidBodyIndexSize);
    readInt(joint.rigidBodyIndexB, data.info.rigidBodyIndexSize);
    readFloat<3>(joint.position.x);
    readFloat<3>(joint.rotation.x);
    readFloat<3>(joint.linearLowerLimit.x);
    readFloat<3>(joint.linearUpperLimit.x);
    readFloat<3>(joint.angularLowerLimit.x);
    readFloat<3>(joint.angularUpperLimit.x);
    readFloat<3>(joint.linearStiffness.x);
    readFloat<3>(joint.angularStiffness.x);
  }
}

void PmxFileLoader::validate(const ModelData &data) const {
  const auto boneCount = static_cast<int32_t>(data.bones.size());
  const auto morphCount = static_cast<int32_t>(data.morphs.size());
  const auto materialCount = static_cast<int32_t>(data.materials.size());
  const auto vertexCount = static_cast<int32_t>(data.vertices.size());
  const auto rigidBodyCount = static_cast<int32_t>(data.rigidBodies.size());

  // Indices are used as array subscripts downstream (PoseSolver, Pose).
  // -1 is the PMX sentinel for "none" and is allowed where noted.
  auto check = [](int32_t index, int32_t count, bool allowNone,
                  const char *what) {
    if (allowNone && index == -1)
      return;
    if (index < 0 || index >= count)
      throw std::runtime_error(std::string("PMX file references an "
                                           "out-of-range ") +
                               what + " index.");
  };

  for (const auto &bone : data.bones) {
    check(bone.parentIndex, boneCount, true, "parent bone");
    if (bone.inheritRotation() || bone.inheritTranslation())
      check(bone.inheritParentIndex, boneCount, true, "inherit parent bone");
  }

  for (const auto &ik : data.ikData) {
    check(ik.targetBoneIndex, boneCount, false, "IK target bone");
    check(ik.endEffector, boneCount, true, "IK end effector bone");
    for (const auto &link : ik.links)
      check(link.boneIndex, boneCount, false, "IK link bone");
  }

  for (const auto &morph : data.morphs) {
    switch (morph.type) {
    case MorphType::Group:
      for (const auto &m : morph.group())
        check(m.index, morphCount, false, "group morph");
      break;
    case MorphType::Vertex:
      for (const auto &m : morph.vertex())
        check(m.index, vertexCount, false, "vertex morph vertex");
      break;
    case MorphType::Bone:
      for (const auto &m : morph.bone())
        check(m.index, boneCount, false, "bone morph bone");
      break;
    case MorphType::UV:
    case MorphType::UV1:
    case MorphType::UV2:
    case MorphType::UV3:
    case MorphType::UV4:
      for (const auto &m : morph.uv())
        check(m.index, vertexCount, false, "UV morph vertex");
      break;
    case MorphType::Material:
      for (const auto &m : morph.material())
        check(m.index, materialCount, true, "material morph material");
      break;
    }
  }

  for (const auto &rb : data.rigidBodies)
    check(rb.boneIndex, boneCount, true, "rigid body bone");

  for (const auto &joint : data.joints) {
    check(joint.rigidBodyIndexA, rigidBodyCount, true, "joint rigid body");
    check(joint.rigidBodyIndexB, rigidBodyCount, true, "joint rigid body");
  }
}

ModelData loadPmxFile(const std::filesystem::path &path) {
  return PmxFileLoader{}.load(path);
}

} // namespace glmmd
