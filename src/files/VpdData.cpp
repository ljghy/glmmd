#include <unordered_map>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/VpdData.h>

namespace glmmd
{

ModelPose VpdData::toModelPose(
    const std::shared_ptr<const ModelData> &modelData) const
{
    ModelPose pose(modelData);

    std::unordered_map<std::string, uint32_t> boneNameToIndex;
    for (uint32_t i = 0; i < modelData->bones.size(); ++i)
        boneNameToIndex.emplace(modelData->bones[i].name, i);

    for (const auto &b : bones)
    {
        auto it = boneNameToIndex.find(codeCvt<ShiftJIS, UTF8>(b.name));
        if (it == boneNameToIndex.end())
            continue;
        auto boneIndex = it->second;

        pose.setLocalBoneTransform(boneIndex, {b.translation, b.rotation});
    }

    return pose;
}

} // namespace glmmd