#ifndef GLMMD_CORE_SHARED_TOON_TEXTURES_H_
#define GLMMD_CORE_SHARED_TOON_TEXTURES_H_

#include <cstdint>
#include <cstddef>

namespace glmmd
{

constexpr int kSharedToonTextureCount = 10;
constexpr int kSharedToonTextureWidth  = 32;
constexpr int kSharedToonTextureHeight = 32;
constexpr size_t kSharedToonTextureSize =
    kSharedToonTextureWidth *
    kSharedToonTextureHeight * 3;

extern const uint8_t
    sharedToonTextureData[kSharedToonTextureCount]
                         [kSharedToonTextureSize];

} // namespace glmmd

#endif
