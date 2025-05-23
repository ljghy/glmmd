#ifndef GLMMD_CORE_MODEL_DATA_H_
#define GLMMD_CORE_MODEL_DATA_H_

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glmmd
{

enum class EncodingMethod : uint8_t
{
    UTF16_LE = 0,
    UTF8     = 1,
};
struct ModelInfo
{
    float version;

    EncodingMethod encodingMethod;

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

enum class VertexSkinningType : uint8_t
{
    BDEF1 = 0,
    BDEF2 = 1,
    BDEF4 = 2,
    SDEF  = 3,
    QDEF  = 4,
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    VertexSkinningType     skinningType;
    std::array<int32_t, 4> boneIndices;
    glm::vec4              boneWeights;
    glm::vec3              sdefC;
    glm::vec3              sdefR0;
    glm::vec3              sdefR1;

    float edgeScale;
};

struct Texture
{
    std::string           rawPath;
    std::filesystem::path path;
};

struct Material
{
    std::string name;
    std::string nameEN;

    glm::vec4 diffuse;
    glm::vec3 specular;
    float     specularPower;
    glm::vec3 ambient;

    uint8_t bitFlag;
    // 0x01 : double sided
    bool doubleSided() const { return bitFlag & 0x01; }
    // 0x02 : ground shadow
    bool groundShadow() const { return bitFlag & 0x02; }
    // 0x04 : cast shadow
    bool castShadow() const { return bitFlag & 0x04; }
    // 0x08 : receive shadow
    bool receiveShadow() const { return bitFlag & 0x08; }
    // 0x10 : render edge
    bool renderEdge() const { return bitFlag & 0x10; }

    glm::vec4 edgeColor;
    float     edgeSize;

    int32_t textureIndex;
    int32_t sphereTextureIndex;
    uint8_t sphereMode; // 0: none, 1: multiply, 2: add

    uint8_t sharedToonFlag; // 0 do not use, 1: use shared toon
    int32_t toonTextureIndex;

    std::string memo;
    int32_t     indicesCount;
};

struct IKLink
{
    int32_t   boneIndex;
    uint8_t   angleLimitFlag;
    glm::vec3 lowerLimit;
    glm::vec3 upperLimit;
};

struct IKData
{
    int32_t targetBoneIndex;

    int32_t endEffector; // end effector
    int32_t loopCount;
    float   limitAngle; // rad

    std::vector<IKLink> links;
};

struct Bone
{
    std::string name;
    std::string nameEN;

    glm::vec3 position;

    int32_t parentIndex;
    int32_t deformLayer;

    uint16_t bitFlag;
    // 0x0001 : bone end representing method | 0: coordinate 1: bone index
    // 0x0002 : allow rotation
    bool allowRotation() const { return bitFlag & 0x0002; }
    // 0x0004 : allow translation
    bool allowTranslation() const { return bitFlag & 0x0004; }
    // 0x0008 : display
    // 0x0010 : allow operation
    // 0x0020 : IK
    bool isIK() const { return bitFlag & 0x0020; }

    // 0x0080 : local inherit
    bool localInherit() const { return bitFlag & 0x0080; }
    // 0x0100 : inherite rotation
    bool inheritRotation() const { return bitFlag & 0x0100; }
    // 0x0200 : inherite translation
    bool inheritTranslation() const { return bitFlag & 0x0200; }

    // 0x0400 : limit axis
    bool limitAxis() const { return bitFlag & 0x0400; }
    // 0x0800 : local axis

    // 0x1000 : deform after physics
    bool deformAfterPhysics() const { return bitFlag & 0x1000; }
    // 0x2000 : external parent

    union
    {
        glm::vec3 endPosition;
        int32_t   endIndex;
    };

    int32_t inheritParentIndex;
    float   inheritWeight;

    glm::vec3 axisDirection;

    glm::vec3 localXVector;
    glm::vec3 localZVector;

    int32_t externalParentKey;

    int32_t ikDataIndex;
};

enum class MorphType : uint8_t
{
    Group    = 0,
    Vertex   = 1,
    Bone     = 2,
    UV       = 3,
    UV1      = 4,
    UV2      = 5,
    UV3      = 6,
    UV4      = 7,
    Material = 8,
};

struct GroupMorph
{
    int32_t index;
    float   ratio;
};

struct VertexMorph
{
    int32_t   index;
    glm::vec3 offset;
};

struct UVMorph
{
    int32_t   index;
    glm::vec4 offset[5];
};

struct BoneMorph
{
    int32_t   index;
    glm::vec3 translation;
    glm::quat rotation;
};

struct MaterialMorph
{
    int32_t   index;
    uint8_t   operation; // 0: multiply, 1: add
    glm::vec4 diffuse;
    glm::vec3 specular;
    float     specularPower;
    glm::vec3 ambient;
    glm::vec4 edgeColor;
    float     edgeSize;
    glm::vec4 texture;
    glm::vec4 sphereTexture;
    glm::vec4 toonTexture;
};

struct Morph
{
    std::string name;
    std::string nameEN;

    uint8_t   panel;
    MorphType type;

    union
    {
        GroupMorph    *group;
        VertexMorph   *vertex;
        UVMorph       *uv;
        BoneMorph     *bone;
        MaterialMorph *material;
    };
    int32_t count;

    void init()
    {
        constexpr size_t dataSize[]{
            sizeof(GroupMorph), sizeof(VertexMorph), sizeof(BoneMorph),
            sizeof(UVMorph),    sizeof(UVMorph),     sizeof(UVMorph),
            sizeof(UVMorph),    sizeof(UVMorph),     sizeof(MaterialMorph)};
        m_buffer.resize((dataSize[static_cast<uint8_t>(type)] * count +
                         sizeof(AlignedStorageType) - 1) /
                        sizeof(AlignedStorageType));
        if (!m_buffer.empty())
        {
            switch (type)
            {
            case MorphType::Group:
                group = std::launder(reinterpret_cast<GroupMorph *>(data()));
                break;
            case MorphType::Vertex:
                vertex = std::launder(reinterpret_cast<VertexMorph *>(data()));
                break;
            case MorphType::Bone:
                bone = std::launder(reinterpret_cast<BoneMorph *>(data()));
                break;
            case MorphType::UV:
            case MorphType::UV1:
            case MorphType::UV2:
            case MorphType::UV3:
            case MorphType::UV4:
                uv = std::launder(reinterpret_cast<UVMorph *>(data()));
                break;
            case MorphType::Material:
                material =
                    std::launder(reinterpret_cast<MaterialMorph *>(data()));
                break;
            }
        }
    }

private:
    char *data() { return m_buffer.empty() ? nullptr : m_buffer[0].data; }

private:
    template <size_t... N>
    struct Max;
    template <size_t N>
    struct Max<N>
    {
        static constexpr size_t value = N;
    };
    template <size_t N, size_t... Rest>
    struct Max<N, Rest...>
    {
        static constexpr size_t value =
            N > Max<Rest...>::value ? N : Max<Rest...>::value;
    };
    template <typename... Types>
    struct AlignedStorage
    {
        static constexpr size_t value = Max<alignof(Types)...>::value;
        struct type
        {
            alignas(value) char data[Max<sizeof(Types)...>::value];
        };
    };

    using AlignedStorageType =
        typename AlignedStorage<GroupMorph, VertexMorph, BoneMorph, UVMorph,
                                MaterialMorph>::type;
    std::vector<AlignedStorageType>
        m_buffer; // aligned allocation guaranteed after C++17
};

struct DisplayFrame
{
    std::string name;
    std::string nameEN;

    uint8_t specialFlag;

    struct Element
    {
        uint8_t type; // 0: bone, 1: morph
        int32_t index;
    };
    std::vector<Element> elements;
};

enum class RigidBodyShape : uint8_t
{
    Sphere  = 0,
    Box     = 1,
    Capsule = 2,
};

enum class PhysicsCalcType : uint8_t
{
    Static  = 0,
    Dynamic = 1,
    Mixed   = 2,
};

struct RigidBody
{
    std::string name;
    std::string nameEN;

    int32_t boneIndex;

    uint8_t  group;
    uint16_t collisionGroupMask;

    RigidBodyShape shape;
    glm::vec3      size;
    glm::vec3      position;
    glm::vec3      rotation;

    float mass;
    float linearDamping;
    float angularDamping;
    float restitution;
    float friction;

    PhysicsCalcType physicsCalcType;
};

enum class JointType : uint8_t
{
    Spring6DOF = 0,
    Normal6DOF = 1,
    P2P        = 2,
    ConeTwist  = 3,
    Slider     = 4,
    Hinge      = 5,
};

struct Joint
{
    std::string name;
    std::string nameEN;

    JointType type;
    int32_t   rigidBodyIndexA;
    int32_t   rigidBodyIndexB;
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

struct ModelData
{
    ModelInfo info;

    std::vector<Vertex>       vertices;
    std::vector<AdditionalUV> additionalUVs;
    std::vector<uint32_t>     indices;
    std::vector<Texture>      textures;
    std::vector<Material>     materials;
    std::vector<IKData>       ikData;
    std::vector<Bone>         bones;
    std::vector<Morph>        morphs;
    std::vector<DisplayFrame> displayFrames;
    std::vector<RigidBody>    rigidBodies;
    std::vector<Joint>        joints;

    void validateIndexByteSizes();
};

} // namespace glmmd

#endif