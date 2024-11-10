#include <glmmd/core/ModelData.h>

namespace glmmd
{

static uint8_t byteSize(size_t n)
{
    return n <= 0xFFu ? 1u : (n <= 0xFFFFu ? 2u : 4u);
}

void ModelData::validateIndexByteSizes()
{
    info.vertexIndexSize    = byteSize(vertices.size());
    info.textureIndexSize   = byteSize(textures.size());
    info.materialIndexSize  = byteSize(materials.size());
    info.boneIndexSize      = byteSize(bones.size());
    info.morphIndexSize     = byteSize(morphs.size());
    info.rigidBodyIndexSize = byteSize(rigidBodies.size());
}

} // namespace glmmd
