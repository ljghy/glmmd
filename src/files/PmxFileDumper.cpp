#include <filesystem>

#include <glmmd/files/PmxFileDumper.h>

namespace glmmd
{

PmxFileDumper::PmxFileDumper(const std::string &filename, bool utf8)
{
    std::filesystem::path path;
    if (utf8)
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

    m_fout.open(path, std::ios::binary);
    if (!m_fout)
        throw std::runtime_error("Failed to open file \"" + filename + "\".");
}

void PmxFileDumper::dump(const ModelData &data)
{
    dumpInfo(data);
    dumpVertices(data);
    dumpIndices(data);
    dumpTextures(data);
    dumpMaterials(data);
    dumpBones(data);
    dumpMorphs(data);
    dumpDisplayFrames(data);
    dumpRigidBodies(data);
    dumpJoints(data);
}

void PmxFileDumper::dumpInfo(const ModelData &data)
{
    const auto &info = data.info;

    m_fout.write("PMX ", 4);
    writeFloat(info.version);
    writeUInt(uint8_t{8});

    m_textEncoding         = info.encodingMethod;
    m_internalTextEncoding = info.internalEncodingMethod;

    for (int i = 0; i < 8; ++i)
        writeUInt(*(&info.encodingMethod + i));

    writeTextBuffer(info.modelName);
    writeTextBuffer(info.modelNameEN);
    writeTextBuffer(info.comment);
    writeTextBuffer(info.commentEN);
}

void PmxFileDumper::dumpVertices(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.vertices.size()));
    for (const auto &v : data.vertices)
    {
        writeFloat<3>(v.position.x);
        writeFloat<3>(v.normal.x);
        writeFloat<2>(v.uv.x);

        for (int i = 0; i < data.info.additionalUVNum; ++i)
            writeFloat<4>(v.additionalUVs[i].x);

        writeUInt(v.skinningType);

        auto sz = data.info.boneIndexSize;
        switch (v.skinningType)
        {
        case VertexSkinningType::BDEF1:
            writeInt(v.boneIndices[0], sz);
            break;
        case VertexSkinningType::BDEF2:
            writeInt(v.boneIndices[0], sz);
            writeInt(v.boneIndices[1], sz);
            writeFloat(v.boneWeights[0]);
            break;
        case VertexSkinningType::BDEF4:
            writeInt(v.boneIndices[0], sz);
            writeInt(v.boneIndices[1], sz);
            writeInt(v.boneIndices[2], sz);
            writeInt(v.boneIndices[3], sz);
            writeFloat<4>(v.boneWeights[0]);
            break;
        case VertexSkinningType::SDEF:
            writeInt(v.boneIndices[0], sz);
            writeInt(v.boneIndices[1], sz);
            writeFloat(v.boneWeights[0]);
            writeFloat<3>(v.sdefC.x);
            writeFloat<3>(v.sdefR0.x);
            writeFloat<3>(v.sdefR1.x);
            break;
        case VertexSkinningType::QDEF:
            writeInt(v.boneIndices[0], sz);
            writeInt(v.boneIndices[1], sz);
            writeInt(v.boneIndices[2], sz);
            writeInt(v.boneIndices[3], sz);
            writeFloat<4>(v.boneWeights[0]);
            break;
        }
        writeFloat(v.edgeScale);
    }
}

void PmxFileDumper::dumpIndices(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.indices.size()));
    for (const auto &i : data.indices)
        writeUInt(i, data.info.vertexIndexSize);
}

void PmxFileDumper::dumpTextures(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.textures.size()));
    for (const auto &t : data.textures)
    {
        writeTextBuffer(t.path);
    }
}

void PmxFileDumper::dumpMaterials(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.materials.size()));
    for (const auto &m : data.materials)
    {
        writeTextBuffer(m.name);
        writeTextBuffer(m.nameEN);

        writeFloat<4>(m.diffuse.x);
        writeFloat<3>(m.specular.x);
        writeFloat(m.specularPower);
        writeFloat<3>(m.ambient.x);
        writeUInt(m.bitFlag);

        writeFloat<4>(m.edgeColor.x);
        writeFloat(m.edgeSize);
        writeInt(m.textureIndex, data.info.textureIndexSize);
        writeInt(m.sphereTextureIndex, data.info.textureIndexSize);
        writeUInt(m.sphereMode);
        writeUInt(m.sharedToonFlag);

        if (m.sharedToonFlag == 0)
            writeInt(m.toonTextureIndex, data.info.textureIndexSize);
        else
            writeInt(m.toonTextureIndex, 1);

        writeTextBuffer(m.memo);
        writeUInt(m.indicesCount);
    }
}

void PmxFileDumper::dumpBones(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.bones.size()));
    for (const auto &b : data.bones)
    {
        writeTextBuffer(b.name);
        writeTextBuffer(b.nameEN);

        writeFloat<3>(b.position.x);
        writeInt(b.parentIndex, data.info.boneIndexSize);
        writeInt(b.deformLayer);
        writeUInt(b.bitFlag);

        if (b.bitFlag & 0x0001)
            writeInt(b.endIndex, data.info.boneIndexSize);
        else
            writeFloat<3>(b.endPosition.x);

        if (b.bitFlag & (0x0100 | 0x0200))
        {
            writeInt(b.inheritParentIndex, data.info.boneIndexSize);
            writeFloat(b.inheritWeight);
        }

        if (b.bitFlag & 0x0400)
            writeFloat<3>(b.axisDirection.x);

        if (b.bitFlag & 0x0800)
        {
            writeFloat<3>(b.localXVector.x);
            writeFloat<3>(b.localZVector.x);
        }

        if (b.bitFlag & 0x2000)
            writeInt(b.externalParentKey);

        if (b.bitFlag & 0x0020)
        {
            const auto &ik = data.ikData[b.ikDataIndex];

            writeInt(ik.targetBoneIndex, data.info.boneIndexSize);
            writeInt(ik.loopCount);
            writeFloat(ik.limitAngle);

            writeUInt(static_cast<uint32_t>(ik.links.size()));
            for (const auto &link : ik.links)
            {
                writeInt(link.boneIndex, data.info.boneIndexSize);
                writeUInt(link.angleLimitFlag);

                if (link.angleLimitFlag)
                {
                    writeFloat<3>(link.lowerLimit.x);
                    writeFloat<3>(link.upperLimit.x);
                }
            }
        }
    }
}

void PmxFileDumper::dumpMorphs(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.morphs.size()));
    for (const auto &m : data.morphs)
    {
        writeTextBuffer(m.name);
        writeTextBuffer(m.nameEN);

        writeUInt(m.panel);
        writeUInt(m.type);

        writeUInt(static_cast<uint32_t>(m.data.size()));
        for (const auto &d : m.data)
        {
            switch (m.type)
            {
            case MorphType::Group:
                dumpGroupMorph(data, d.group);
                break;
            case MorphType::Vertex:
                dumpVertexMorph(data, d.vertex);
                break;
            case MorphType::Bone:
                dumpBoneMorph(data, d.bone);
                break;
            case MorphType::UV:
            case MorphType::UV1:
            case MorphType::UV2:
            case MorphType::UV3:
            case MorphType::UV4:
                dumpUVMorph(data, d.uv,
                            static_cast<uint8_t>(m.type) -
                                static_cast<uint8_t>(MorphType::UV));
                break;
            case MorphType::Material:
                dumpMaterialMorph(data, d.material);
                break;
            }
        }
    }
}

void PmxFileDumper::dumpGroupMorph(const ModelData  &data,
                                   const GroupMorph &morph)
{
    writeInt(morph.index, data.info.morphIndexSize);
    writeFloat(morph.ratio);
}

void PmxFileDumper::dumpVertexMorph(const ModelData   &data,
                                    const VertexMorph &morph)
{
    writeInt(morph.index, data.info.vertexIndexSize);
    writeFloat<3>(morph.offset.x);
}

void PmxFileDumper::dumpBoneMorph(const ModelData &data, const BoneMorph &morph)
{
    writeInt(morph.index, data.info.boneIndexSize);
    writeFloat<3>(morph.translation.x);
    writeFloat<4>(morph.rotation[0]);
}

void PmxFileDumper::dumpUVMorph(const ModelData &data, const UVMorph &morph,
                                uint8_t num)
{
    writeInt(morph.index, data.info.vertexIndexSize);
    writeFloat<4>(morph.offset[num].x);
}

void PmxFileDumper::dumpMaterialMorph(const ModelData     &data,
                                      const MaterialMorph &morph)
{
    writeInt(morph.index, data.info.materialIndexSize);
    writeUInt(morph.operation);
    writeFloat<4>(morph.diffuse.x);
    writeFloat<3>(morph.specular.x);
    writeFloat(morph.specularPower);
    writeFloat<3>(morph.ambient.x);
    writeFloat<4>(morph.edgeColor.x);
    writeFloat(morph.edgeSize);
    writeFloat<4>(morph.texture.x);
    writeFloat<4>(morph.sphereTexture.x);
    writeFloat<4>(morph.toonTexture.x);
}

void PmxFileDumper::dumpDisplayFrames(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.displayFrames.size()));
    for (const auto &f : data.displayFrames)
    {
        writeTextBuffer(f.name);
        writeTextBuffer(f.nameEN);
        writeUInt(f.specialFlag);
        writeUInt(static_cast<uint32_t>(f.elements.size()));
        for (const auto &e : f.elements)
        {
            writeUInt(e.type);
            writeInt(e.index, e.type == 0 ? data.info.boneIndexSize
                                          : data.info.morphIndexSize);
        }
    }
}

void PmxFileDumper::dumpRigidBodies(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.rigidBodies.size()));
    for (const auto &r : data.rigidBodies)
    {
        writeTextBuffer(r.name);
        writeTextBuffer(r.nameEN);
        writeInt(r.boneIndex, data.info.boneIndexSize);
        writeUInt(r.group);
        writeUInt(r.collisionGroupMask);
        writeUInt(r.shape);
        writeFloat<3>(r.size.x);
        writeFloat<3>(r.position.x);
        writeFloat<3>(r.rotation.x);
        writeFloat(r.mass);
        writeFloat(r.linearDamping);
        writeFloat(r.angularDamping);
        writeFloat(r.restitution);
        writeFloat(r.friction);
        writeUInt(r.physicsCalcType);
    }
}

void PmxFileDumper::dumpJoints(const ModelData &data)
{
    writeUInt(static_cast<uint32_t>(data.joints.size()));
    for (const auto &j : data.joints)
    {
        writeTextBuffer(j.name);
        writeTextBuffer(j.nameEN);
        writeUInt(j.type);
        writeInt(j.rigidBodyIndexA, data.info.rigidBodyIndexSize);
        writeInt(j.rigidBodyIndexB, data.info.rigidBodyIndexSize);
        writeFloat<3>(j.position.x);
        writeFloat<3>(j.rotation.x);
        writeFloat<3>(j.linearLowerLimit.x);
        writeFloat<3>(j.linearUpperLimit.x);
        writeFloat<3>(j.angularLowerLimit.x);
        writeFloat<3>(j.angularUpperLimit.x);
        writeFloat<3>(j.linearStiffness.x);
        writeFloat<3>(j.angularStiffness.x);
    }
}

} // namespace glmmd