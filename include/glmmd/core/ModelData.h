#ifndef GLMMD_CORE_MODEL_DATA_H_
#define GLMMD_CORE_MODEL_DATA_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace glmmd {

// Enables type-safe bitwise operations on a scoped enum used as a flag set.
#define GLMMD_DEFINE_FLAG_OPERATORS(Enum, Underlying)                          \
  constexpr Enum operator|(Enum a, Enum b) {                                   \
    return static_cast<Enum>(static_cast<Underlying>(a) |                      \
                             static_cast<Underlying>(b));                      \
  }                                                                            \
  constexpr Enum operator&(Enum a, Enum b) {                                   \
    return static_cast<Enum>(static_cast<Underlying>(a) &                      \
                             static_cast<Underlying>(b));                      \
  }                                                                            \
  constexpr Enum &operator|=(Enum &a, Enum b) { return a = a | b; }            \
  constexpr bool any(Enum a) { return static_cast<Underlying>(a) != 0; }

enum class Encoding : uint8_t {
  UTF16_LE = 0,
  UTF8 = 1,
};
struct ModelInfo {
  float version;

  Encoding encoding;

  uint8_t additionalUVNum;
  uint8_t vertexIndexSize;
  uint8_t textureIndexSize;
  uint8_t materialIndexSize;
  uint8_t boneIndexSize;
  uint8_t morphIndexSize;
  uint8_t rigidBodyIndexSize;

  std::string modelName;
  std::string modelNameEN;
  std::string comment;
  std::string commentEN;
};

enum class SkinningType : uint8_t {
  BDEF1 = 0,
  BDEF2 = 1,
  BDEF4 = 2,
  SDEF = 3,
  QDEF = 4,
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  SkinningType skinningType;
  std::array<int32_t, 4> boneIndices{0, 0, 0, 0};
  glm::vec4 boneWeights{0.f};
  glm::vec3 sdefC;
  glm::vec3 sdefR0;
  glm::vec3 sdefR1;

  float edgeScale;
};

enum class MaterialFlag : uint8_t {
  None = 0,
  DoubleSided = 0x01,
  GroundShadow = 0x02,
  CastShadow = 0x04,
  ReceiveShadow = 0x08,
  RenderEdge = 0x10,
};
GLMMD_DEFINE_FLAG_OPERATORS(MaterialFlag, uint8_t)

enum class SphereMode : uint8_t {
  None = 0,
  Multiply = 1,
  Add = 2,
};

struct Material {
  std::string name;
  std::string nameEN;

  glm::vec4 diffuse;
  glm::vec3 specular;
  float specularPower;
  glm::vec3 ambient;

  MaterialFlag flags;
  bool doubleSided() const { return any(flags & MaterialFlag::DoubleSided); }
  bool groundShadow() const { return any(flags & MaterialFlag::GroundShadow); }
  bool castShadow() const { return any(flags & MaterialFlag::CastShadow); }
  bool receiveShadow() const {
    return any(flags & MaterialFlag::ReceiveShadow);
  }
  bool renderEdge() const { return any(flags & MaterialFlag::RenderEdge); }

  glm::vec4 edgeColor;
  float edgeSize;

  int32_t textureIndex;
  int32_t sphereTextureIndex;
  SphereMode sphereMode;

  uint8_t sharedToonFlag; // 0 do not use, 1: use shared toon
  int32_t toonTextureIndex;

  std::string memo;
  int32_t indicesCount;
};

struct IKLink {
  int32_t boneIndex;
  uint8_t angleLimitFlag;
  glm::vec3 lowerLimit;
  glm::vec3 upperLimit;
};

struct IKData {
  int32_t targetBoneIndex;

  int32_t endEffector; // end effector
  int32_t loopCount;
  float limitAngle; // rad

  std::vector<IKLink> links;
};

enum class BoneFlag : uint16_t {
  None = 0,
  EndByBoneIndex = 0x0001, // bone end: 0 = coordinate, 1 = bone index
  AllowRotation = 0x0002,
  AllowTranslation = 0x0004,
  Display = 0x0008,
  AllowOperation = 0x0010,
  IK = 0x0020,
  LocalInherit = 0x0080,
  InheritRotation = 0x0100,
  InheritTranslation = 0x0200,
  LimitAxis = 0x0400,
  LocalAxis = 0x0800,
  DeformAfterPhysics = 0x1000,
  ExternalParent = 0x2000,
};
GLMMD_DEFINE_FLAG_OPERATORS(BoneFlag, uint16_t)

struct Bone {
  std::string name;
  std::string nameEN;

  glm::vec3 position;

  int32_t parentIndex;
  int32_t deformLayer;

  BoneFlag flags;
  bool endByBoneIndex() const { return any(flags & BoneFlag::EndByBoneIndex); }
  bool allowRotation() const { return any(flags & BoneFlag::AllowRotation); }
  bool allowTranslation() const {
    return any(flags & BoneFlag::AllowTranslation);
  }
  bool isIK() const { return any(flags & BoneFlag::IK); }
  bool localInherit() const { return any(flags & BoneFlag::LocalInherit); }
  bool inheritRotation() const {
    return any(flags & BoneFlag::InheritRotation);
  }
  bool inheritTranslation() const {
    return any(flags & BoneFlag::InheritTranslation);
  }
  bool limitAxis() const { return any(flags & BoneFlag::LimitAxis); }
  bool localAxis() const { return any(flags & BoneFlag::LocalAxis); }
  bool deformAfterPhysics() const {
    return any(flags & BoneFlag::DeformAfterPhysics);
  }
  bool externalParent() const { return any(flags & BoneFlag::ExternalParent); }

  union {
    glm::vec3 endPosition;
    int32_t endIndex;
  };

  int32_t inheritParentIndex;
  float inheritWeight;

  glm::vec3 axisDirection;

  glm::vec3 localXVector;
  glm::vec3 localZVector;

  int32_t externalParentKey;

  int32_t ikDataIndex;
};

enum class MorphType : uint8_t {
  Group = 0,
  Vertex = 1,
  Bone = 2,
  UV = 3,
  UV1 = 4,
  UV2 = 5,
  UV3 = 6,
  UV4 = 7,
  Material = 8,
};

struct GroupMorph {
  int32_t index;
  float weight;
};

struct VertexMorph {
  int32_t index;
  glm::vec3 offset;
};

struct UVMorph {
  int32_t index;
  glm::vec4 offset[5];
};

struct BoneMorph {
  int32_t index;
  glm::vec3 translation;
  glm::quat rotation;
};

enum class MaterialMorphOperation : uint8_t {
  Multiply = 0,
  Add = 1,
};

struct MaterialMorph {
  int32_t index;
  MaterialMorphOperation operation;
  glm::vec4 diffuse;
  glm::vec3 specular;
  float specularPower;
  glm::vec3 ambient;
  glm::vec4 edgeColor;
  float edgeSize;
  glm::vec4 texture;
  glm::vec4 sphereTexture;
  glm::vec4 toonTexture;
};

// The active alternative is determined by Morph::type:
//   Group                       -> std::vector<GroupMorph>
//   Vertex                      -> std::vector<VertexMorph>
//   Bone                        -> std::vector<BoneMorph>
//   UV, UV1, UV2, UV3, UV4      -> std::vector<UVMorph>
//   Material                    -> std::vector<MaterialMorph>
using MorphData =
    std::variant<std::monostate, std::vector<GroupMorph>,
                 std::vector<VertexMorph>, std::vector<BoneMorph>,
                 std::vector<UVMorph>, std::vector<MaterialMorph>>;

struct Morph {
  std::string name;
  std::string nameEN;

  uint8_t panel;
  MorphType type;

  MorphData data;

  std::vector<GroupMorph> &group() {
    return std::get<std::vector<GroupMorph>>(data);
  }
  const std::vector<GroupMorph> &group() const {
    return std::get<std::vector<GroupMorph>>(data);
  }

  std::vector<VertexMorph> &vertex() {
    return std::get<std::vector<VertexMorph>>(data);
  }
  const std::vector<VertexMorph> &vertex() const {
    return std::get<std::vector<VertexMorph>>(data);
  }

  std::vector<BoneMorph> &bone() {
    return std::get<std::vector<BoneMorph>>(data);
  }
  const std::vector<BoneMorph> &bone() const {
    return std::get<std::vector<BoneMorph>>(data);
  }

  std::vector<UVMorph> &uv() { return std::get<std::vector<UVMorph>>(data); }
  const std::vector<UVMorph> &uv() const {
    return std::get<std::vector<UVMorph>>(data);
  }

  std::vector<MaterialMorph> &material() {
    return std::get<std::vector<MaterialMorph>>(data);
  }
  const std::vector<MaterialMorph> &material() const {
    return std::get<std::vector<MaterialMorph>>(data);
  }
};

enum class DisplayElementType : uint8_t {
  Bone = 0,
  Morph = 1,
};

struct DisplayFrame {
  std::string name;
  std::string nameEN;

  uint8_t specialFlag;

  struct Element {
    DisplayElementType type;
    int32_t index;
  };
  std::vector<Element> elements;
};

enum class RigidBodyShape : uint8_t {
  Sphere = 0,
  Box = 1,
  Capsule = 2,
};

enum class PhysicsCalcType : uint8_t {
  Static = 0,
  Dynamic = 1,
  Mixed = 2,
};

struct RigidBody {
  std::string name;
  std::string nameEN;

  int32_t boneIndex;

  uint8_t group;
  uint16_t collisionGroupMask;

  RigidBodyShape shape;
  glm::vec3 size;
  glm::vec3 position;
  glm::vec3 rotation;

  float mass;
  float linearDamping;
  float angularDamping;
  float restitution;
  float friction;

  PhysicsCalcType physicsCalcType;

  float getSphereRadius() const { return size.x; }

  float getCapsuleRadius() const { return size.x; }
  float getCapsuleHeight() const { return size.y; }

  glm::vec3 getBoxHalfExtents() const { return size; }
};

enum class JointType : uint8_t {
  Spring6DOF = 0,
  Normal6DOF = 1,
  P2P = 2,
  ConeTwist = 3,
  Slider = 4,
  Hinge = 5,
};

struct Joint {
  std::string name;
  std::string nameEN;

  JointType type;
  int32_t rigidBodyIndexA;
  int32_t rigidBodyIndexB;
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 linearLowerLimit;
  glm::vec3 linearUpperLimit;
  glm::vec3 angularLowerLimit;
  glm::vec3 angularUpperLimit;
  glm::vec3 linearStiffness;
  glm::vec3 angularStiffness;
};

using AdditionalUV = std::array<glm::vec4, 4>;

// Immutable, parsed representation of a PMX model: geometry, materials, bone
// hierarchy, morphs, and physics definitions. Typically wrapped in a
// shared_ptr and shared by Pose, PoseSolver, and ModelRenderData. Produced by
// loadPmxFile.
struct ModelData {
  std::filesystem::path baseDir;

  ModelInfo info;

  std::vector<Vertex> vertices;
  std::vector<AdditionalUV> additionalUVs;
  std::vector<uint32_t> indices;
  std::vector<std::string> texturePaths;
  std::vector<Material> materials;
  std::vector<IKData> ikData;
  std::vector<Bone> bones;
  std::vector<Morph> morphs;
  std::vector<DisplayFrame> displayFrames;
  std::vector<RigidBody> rigidBodies;
  std::vector<Joint> joints;
};

} // namespace glmmd

#endif