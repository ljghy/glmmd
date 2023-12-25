#include <stdexcept>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb/stb_image.h>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/PmxFileLoader.h>

namespace glmmd
{

std::shared_ptr<ModelData> PmxFileLoader::load(const std::string &filename,
                                               bool               utf8Path)
{
    std::filesystem::path path;
    if (utf8Path)
    {
        path =
#if __cplusplus <= 201703L
            std::filesystem::u8path(filename);
#else
            reinterpret_cast<const char8_t *>(filename.data());
#endif
    }
    else
        path = filename;

    m_fin.open(path, std::ios::binary);
    if (!m_fin)
        throw std::runtime_error("Failed to open file \"" + filename + "\".");
    m_modelDir = path.make_preferred().parent_path();

    auto data = std::make_shared<ModelData>();

    loadInfo(*data);
    loadVertices(*data);
    loadIndices(*data);
    loadTextures(*data);
    loadMaterials(*data);
    loadBones(*data);
    loadMorphs(*data);
    loadDisplayFrames(*data);
    loadRigidBodies(*data);
    loadJoints(*data);

    return data;
}

void PmxFileLoader::loadInfo(ModelData &data)
{
    ModelInfo &info = data.info;

    char header[4];
    m_fin.read(header, 4);
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

    readUInt(info.encodingMethod);
    readUInt(info.additionalUVNum);
    readUInt(info.vertexIndexSize);
    readUInt(info.textureIndexSize);
    readUInt(info.materialIndexSize);
    readUInt(info.boneIndexSize);
    readUInt(info.morphIndexSize);
    readUInt(info.rigidBodyIndexSize);

#ifndef GLMMD_DO_NOT_FORCE_UTF8
    info.internalEncodingMethod = EncodingMethod::UTF8; // UTF-8
#else
    info.internalEncodingMethod = info.encodingMethod;
#endif

    readTextBuffer(info.modelName);
    readTextBuffer(info.modelNameEN);
    readTextBuffer(info.comment);
    readTextBuffer(info.commentEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
    if (info.encodingMethod == EncodingMethod::UTF16_LE)
    {
        info.modelName   = codeCvt<UTF16_LE, UTF8>(info.modelName);
        info.modelNameEN = codeCvt<UTF16_LE, UTF8>(info.modelNameEN);
        info.comment     = codeCvt<UTF16_LE, UTF8>(info.comment);
        info.commentEN   = codeCvt<UTF16_LE, UTF8>(info.commentEN);
    }
#endif
}

void PmxFileLoader::loadVertices(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.vertices.resize(count);

    for (auto &vert : data.vertices)
    {
        readFloat<3>(vert.position.x);
        readFloat<3>(vert.normal.x);
        readFloat<2>(vert.uv.x);

        if (vert.uv.x > 1.f || vert.uv.x < 0.f)
            vert.uv.x = vert.uv.x - std::floor(vert.uv.x);
        if (vert.uv.y > 1.f || vert.uv.y < 0.f)
            vert.uv.y = vert.uv.y - std::floor(vert.uv.y);

        for (int i = 0; i < data.info.additionalUVNum; ++i)
            readFloat<4>(vert.additionalUVs[i].x);

        readUInt(vert.skinningType);

        auto &sz = data.info.boneIndexSize;
        switch (vert.skinningType)
        {
        case VertexSkinningType::BDEF1:
            readInt(vert.boneIndices[0], sz);
            break;
        case VertexSkinningType::BDEF2:
            readInt(vert.boneIndices[0], sz);
            readInt(vert.boneIndices[1], sz);
            readFloat<1>(vert.boneWeights[0]);
            break;
        case VertexSkinningType::BDEF4:
            readInt(vert.boneIndices[0], sz);
            readInt(vert.boneIndices[1], sz);
            readInt(vert.boneIndices[2], sz);
            readInt(vert.boneIndices[3], sz);
            readFloat<4>(vert.boneWeights[0]);
            break;
        case VertexSkinningType::SDEF:
            readInt(vert.boneIndices[0], sz);
            readInt(vert.boneIndices[1], sz);
            readFloat<1>(vert.boneWeights[0]);
            readFloat<3>(vert.sdefC.x);
            readFloat<3>(vert.sdefR0.x);
            readFloat<3>(vert.sdefR1.x);
            break;
        case VertexSkinningType::QDEF:
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

void PmxFileLoader::loadIndices(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.indices.resize(count);

    for (auto &index : data.indices)
        readUInt(index, data.info.vertexIndexSize);
}

void PmxFileLoader::loadTextures(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.textures.resize(count);

    for (auto &texture : data.textures)
    {
        readTextBuffer(texture.path);

        std::string u8path =
            data.info.encodingMethod == EncodingMethod::UTF16_LE
                ? codeCvt<UTF16_LE, UTF8>(texture.path)
                : texture.path;

#ifndef _WIN32
        std::replace(u8path.begin(), u8path.end(), '\\', '/');
#endif
        std::filesystem::path path =
#if __cplusplus <= 201703L
            std::filesystem::u8path(u8path);
#else
            reinterpret_cast<const char8_t *>(u8path.c_str());
#endif
        path = m_modelDir / path.make_preferred();

        stbi_uc *pixels =
            stbi_load(reinterpret_cast<const char *>(path.u8string().data()),
                      &texture.width, &texture.height, nullptr, 4);
        if (!pixels)
        {
            texture.exists = false;
        }
        else
        {
            texture.exists   = true;
            texture.channels = 4;
            texture.data.reset(pixels, stbi_image_free);
        }
    }
}

void PmxFileLoader::loadMaterials(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.materials.resize(count);

    for (auto &mat : data.materials)
    {
        readTextBuffer(mat.name);
        readTextBuffer(mat.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            mat.name   = codeCvt<UTF16_LE, UTF8>(mat.name);
            mat.nameEN = codeCvt<UTF16_LE, UTF8>(mat.nameEN);
        }
#endif

        readFloat<4>(mat.diffuse.x);
        readFloat<3>(mat.specular.x);
        readFloat(mat.specularPower);
        readFloat<3>(mat.ambient.x);
        readUInt(mat.bitFlag);

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
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
            mat.memo = codeCvt<UTF16_LE, UTF8>(mat.memo);
#endif
        readInt(mat.indicesCount);
    }
}

void PmxFileLoader::loadBones(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.bones.resize(count);

    int32_t i = 0;
    for (auto &bone : data.bones)
    {
        readTextBuffer(bone.name);
        readTextBuffer(bone.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            bone.name   = codeCvt<UTF16_LE, UTF8>(bone.name);
            bone.nameEN = codeCvt<UTF16_LE, UTF8>(bone.nameEN);
        }
#endif
        readFloat<3>(bone.position.x);
        readInt(bone.parentIndex, data.info.boneIndexSize);
        readInt(bone.deformLayer);

        readUInt(bone.bitFlag);

        if (bone.bitFlag & 0x0001)
            readInt(bone.endIndex, data.info.boneIndexSize);
        else
            readFloat<3>(bone.endPosition.x);

        if (bone.bitFlag & (0x0100 | 0x0200))
        {
            readInt(bone.inheritParentIndex, data.info.boneIndexSize);
            readFloat(bone.inheritWeight);
        }

        if (bone.bitFlag & 0x0400)
            readFloat<3>(bone.axisDirection.x);

        if (bone.bitFlag & 0x0800)
        {
            readFloat<3>(bone.localXVector.x);
            readFloat<3>(bone.localZVector.x);
        }

        if (bone.bitFlag & 0x2000)
            readInt(bone.externalParentKey);

        if (bone.bitFlag & 0x0020)
        {
            bone.ikDataIndex = static_cast<int32_t>(data.ikData.size());
            data.ikData.emplace_back();
            auto &ik = data.ikData.back();

            ik.realTargetBoneIndex = i;
            readInt(ik.targetBoneIndex, data.info.boneIndexSize);
            readInt(ik.loopCount);
            readFloat(ik.limitAngle);

            int32_t ikLinkCount;
            readInt(ikLinkCount);
            ik.links.resize(ikLinkCount);
            for (auto &link : ik.links)
            {
                readInt(link.boneIndex, data.info.boneIndexSize);
                readUInt(link.angleLimitFlag);
                if (link.angleLimitFlag)
                {
                    readFloat<3>(link.lowerLimit.x);
                    readFloat<3>(link.upperLimit.x);
                }
            }
        }

#ifndef GLMMD_DO_NOT_FORCE_UTF8
        data.u8BoneNameToIndex[bone.name] = i++;
#else
        data.u8BoneNameToIndex[data.info.encodingMethod == EncodingMethod::UTF8
                                   ? bone.name
                                   : CodeCvt::UTF16_LE_to_UTF8(bone.name)] =
            i++;
#endif
    }
}

void PmxFileLoader::loadMorphs(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.morphs.resize(count);

    int32_t i = 0;
    for (auto &morph : data.morphs)
    {
        readTextBuffer(morph.name);
        readTextBuffer(morph.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            morph.name   = codeCvt<UTF16_LE, UTF8>(morph.name);
            morph.nameEN = codeCvt<UTF16_LE, UTF8>(morph.nameEN);
        }
#endif
        readUInt(morph.panel);
        readUInt(morph.type);

        int32_t dataCount;
        readInt(dataCount);
        morph.data.reserve(dataCount);
        for (int32_t j = 0; j < dataCount; ++j)
        {
            switch (morph.type)
            {
            case MorphType::Group:
                morph.data.emplace_back(loadGroupMorph(data));
                break;
            case MorphType::Vertex:
                morph.data.emplace_back(loadVertexMorph(data));
                break;
            case MorphType::Bone:
                morph.data.emplace_back(loadBoneMorph(data));
                break;
            case MorphType::UV:
            case MorphType::UV1:
            case MorphType::UV2:
            case MorphType::UV3:
            case MorphType::UV4:
                morph.data.emplace_back(
                    loadUVMorph(data, static_cast<uint8_t>(morph.type) -
                                          static_cast<uint8_t>(MorphType::UV)));
                break;
            case MorphType::Material:
                morph.data.emplace_back(loadMaterialMorph(data));
                break;
            }
        }
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        data.u8MorphNameToIndex[morph.name] = i++;
#else
        data.u8MorphNameToIndex[data.info.encodingMethod == EncodingMethod::UTF8
                                    ? morph.name
                                    : CodeCvt::UTF16_LE_to_UTF8(morph.name)] =
            i++;
#endif
    }
}

Morph::MorphData PmxFileLoader::loadGroupMorph(ModelData &data)
{
    Morph::MorphData morph;
    auto            &group = morph.group;
    readUInt(group.index, data.info.morphIndexSize);
    readFloat(group.ratio);
    return morph;
}

Morph::MorphData PmxFileLoader::loadVertexMorph(ModelData &data)
{
    Morph::MorphData morph;
    auto            &vertex = morph.vertex;
    readUInt(vertex.index, data.info.vertexIndexSize);
    readFloat<3>(vertex.offset.x);
    return morph;
}

Morph::MorphData PmxFileLoader::loadBoneMorph(ModelData &data)
{
    Morph::MorphData morph;
    auto            &bone = morph.bone;
    readUInt(bone.index, data.info.boneIndexSize);
    readFloat<3>(bone.translation.x);
    glm::vec4 q;
    readFloat<4>(q.x); // internal order: x, y, z, w
    bone.rotation = glm::quat(q.w, q.x, q.y, q.z);
    return morph;
}

Morph::MorphData PmxFileLoader::loadUVMorph(ModelData &data, uint8_t num)
{
    Morph::MorphData morph;
    auto            &uv = morph.uv;
    for (uint8_t i = 0; i < 5; ++i)
        if (i != num)
            uv.offset[num] = glm::vec4(0.f);
    readUInt(uv.index, data.info.vertexIndexSize);
    readFloat<4>(uv.offset[num].x);
    return morph;
}

Morph::MorphData PmxFileLoader::loadMaterialMorph(ModelData &data)
{
    Morph::MorphData morph;
    auto            &mat = morph.material;
    readUInt(mat.index, data.info.materialIndexSize);
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
    return morph;
}

void PmxFileLoader::loadDisplayFrames(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.displayFrames.resize(count);

    for (auto &frame : data.displayFrames)
    {
        readTextBuffer(frame.name);
        readTextBuffer(frame.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            frame.name   = codeCvt<UTF16_LE, UTF8>(frame.name);
            frame.nameEN = codeCvt<UTF16_LE, UTF8>(frame.nameEN);
        }
#endif

        readUInt(frame.specialFlag);

        int32_t elementCount;
        readInt(elementCount);
        frame.elements.resize(elementCount);
        for (auto &element : frame.elements)
        {
            readUInt(element.type);
            readInt(element.index, element.type == 0
                                       ? data.info.boneIndexSize
                                       : data.info.morphIndexSize);
        }
    }
}

void PmxFileLoader::loadRigidBodies(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.rigidBodies.resize(count);

    for (auto &rigidBody : data.rigidBodies)
    {
        readTextBuffer(rigidBody.name);
        readTextBuffer(rigidBody.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            rigidBody.name   = codeCvt<UTF16_LE, UTF8>(rigidBody.name);
            rigidBody.nameEN = codeCvt<UTF16_LE, UTF8>(rigidBody.nameEN);
        }
#endif

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

void PmxFileLoader::loadJoints(ModelData &data)
{
    int32_t count;
    readInt(count);
    data.joints.resize(count);

    for (auto &joint : data.joints)
    {
        readTextBuffer(joint.name);
        readTextBuffer(joint.nameEN);
#ifndef GLMMD_DO_NOT_FORCE_UTF8
        if (data.info.encodingMethod == EncodingMethod::UTF16_LE)
        {
            joint.name   = codeCvt<UTF16_LE, UTF8>(joint.name);
            joint.nameEN = codeCvt<UTF16_LE, UTF8>(joint.nameEN);
        }
#endif

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

} // namespace glmmd