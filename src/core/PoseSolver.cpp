#include <glmmd/core/PoseSolver.h>

#include "ModelPhysicsImpl.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace glmmd {

PoseSolver::PoseSolver(const ModelData &modelData) { create(modelData); }

void PoseSolver::create(const ModelData &modelData) {
  m_modelData = &modelData;

  const auto count = modelData.bones.size();
  m_boneLocalOffsets.resize(count);
  m_boneDeformOrder.resize(count);
  m_hierarchyOrder.clear();
  m_hierarchyOrder.reserve(count);

  std::vector<std::vector<uint32_t>> children(count);

  for (uint32_t i = 0; i < count; ++i) {
    const auto &bone = modelData.bones[i];
    if (bone.parentIndex < 0) {
      m_hierarchyOrder.push_back(i);
      m_boneLocalOffsets[i] = bone.position;
    } else {
      children[bone.parentIndex].push_back(i);
      m_boneLocalOffsets[i] =
          bone.position - modelData.bones[bone.parentIndex].position;
    }
  }

  for (size_t i = 0; i < m_hierarchyOrder.size(); ++i) {
    const auto &list = children[m_hierarchyOrder[i]];
    m_hierarchyOrder.insert(m_hierarchyOrder.end(), list.begin(), list.end());
  }

  // Prevent ancestor walks from looping forever on a cyclic hierarchy.
  if (m_hierarchyOrder.size() != count)
    throw std::invalid_argument("Cyclic bone hierarchy");

  std::iota(m_boneDeformOrder.begin(), m_boneDeformOrder.end(), 0);
  std::sort(m_boneDeformOrder.begin(), m_boneDeformOrder.end(),
            [&](uint32_t a, uint32_t b) {
              const auto &ba = modelData.bones[a];
              const auto &bb = modelData.bones[b];
              return std::tuple(ba.deformAfterPhysics(), ba.deformLayer, a) <
                     std::tuple(bb.deformAfterPhysics(), bb.deformLayer, b);
            });

  m_afterPhysicsOffset = static_cast<uint32_t>(
      std::find_if(
          m_boneDeformOrder.begin(), m_boneDeformOrder.end(),
          [&](uint32_t i) { return modelData.bones[i].deformAfterPhysics(); }) -
      m_boneDeformOrder.begin());

  auto pathTo = [&](int32_t bone) {
    std::vector<uint32_t> path;
    for (; bone >= 0; bone = modelData.bones[bone].parentIndex)
      path.push_back(bone);

    std::reverse(path.begin(), path.end());
    return path;
  };

  m_ikPlans.clear();
  uint32_t scratchOffset = 0;

  for (const auto &ik : modelData.ikData) {
    auto &plan = m_ikPlans.emplace_back();
    plan.scratchOffset = scratchOffset;
    scratchOffset += static_cast<uint32_t>(ik.links.size());
    plan.effectorPath = pathTo(ik.endEffector);

    float chainLength = 0.f;
    int32_t previous = ik.endEffector;
    for (const auto &link : ik.links) {
      auto &lp = plan.links.emplace_back();
      lp.parentPath = pathTo(modelData.bones[link.boneIndex].parentIndex);

      if (previous >= 0)
        chainLength += glm::distance(modelData.bones[previous].position,
                                     modelData.bones[link.boneIndex].position);
      previous = link.boneIndex;

      if (!link.angleLimitFlag)
        continue;

      lp.fixed = link.lowerLimit == link.upperLimit;
      for (int axis = 0; axis < 3 && !lp.fixed; ++axis) {
        const int a = (axis + 1) % 3, b = (axis + 2) % 3;
        if (link.lowerLimit[a] == 0.f && link.upperLimit[a] == 0.f &&
            link.lowerLimit[b] == 0.f && link.upperLimit[b] == 0.f)
          lp.hingeAxis = axis;
      }
    }

    plan.positionTolerance = std::max(1e-8f, chainLength * 1e-5f);
  }
}

void PoseSolver::solveBeforePhysics(Pose &pose) const {
  pose.evaluateMorphWeights();

  for (size_t i = 0; i < pose.m_boneStates.size(); ++i) {
    auto &state = pose.m_boneStates[i];
    state = {};
    state.animated = pose.m_localBoneTransforms[i];
  }

  applyBoneMorphs(pose);
  for (auto &state : pose.m_boneStates)
    state.inherited = state.animated;

  solveBoneRange(pose, 0, m_afterPhysicsOffset);

  updateGlobalTransforms(pose);
}

void PoseSolver::solveAfterPhysics(Pose &pose) const {
  solveBoneRange(pose, m_afterPhysicsOffset,
                 static_cast<uint32_t>(m_boneDeformOrder.size()));

  updateGlobalTransforms(pose);
}

void PoseSolver::solveBoneRange(Pose &pose, uint32_t first,
                                uint32_t last) const {
  for (; first < last; ++first) {
    const auto index = m_boneDeformOrder[first];
    const auto &bone = m_modelData->bones[index];

    updateInheritedTransform(pose, index);
    ensureGlobalTransform(pose, index);

    if (m_enableIK && bone.isIK())
      solveIK(pose, bone.ikDataIndex);
  }
}

void PoseSolver::applyBoneMorphs(Pose &pose) const {
  for (uint32_t i = 0; i < pose.m_effectiveMorphWeights.size(); ++i) {
    const auto &morph = m_modelData->morphs[i];
    const float weight = pose.m_effectiveMorphWeights[i];

    if (morph.type != MorphType::Bone || weight == 0.f)
      continue;

    for (const auto &entry : morph.bone()) {
      auto &animated = pose.m_boneStates[entry.index].animated;
      animated.translation += weight * entry.translation;
      animated.rotation = glm::normalize(
          animated.rotation *
          glm::slerp(glm::identity<glm::quat>(), entry.rotation, weight));
    }
  }
}

void PoseSolver::updateInheritedTransform(Pose &pose, uint32_t index) const {
  const auto &bone = m_modelData->bones[index];
  auto value = pose.m_boneStates[index].animated;
  const auto source = bone.inheritParentIndex;

  if ((bone.inheritRotation() || bone.inheritTranslation()) && source >= 0 &&
      bone.inheritWeight != 0.f) {
    Transform grant;
    if (bone.localInherit()) {
      ensureGlobalTransform(pose, source);
      grant = pose.m_globalBoneTransforms[source];
      grant.translation -= m_modelData->bones[source].position;
    } else {
      // Saved grants include animation and morphs, but exclude the source's own
      // IK: sample that correction at this event's position in deformation
      // order.
      if (pose.m_boneStates[source].physicsChannels)
        ensureGlobalTransform(pose, source);

      grant = pose.m_boneStates[source].inherited;
      grant.rotation = pose.m_boneStates[source].ikRotation * grant.rotation;
    }

    if (bone.inheritRotation())
      value.rotation = glm::normalize(
          value.rotation * glm::slerp(glm::identity<glm::quat>(),
                                      grant.rotation, bone.inheritWeight));
    if (bone.inheritTranslation())
      value.translation += bone.inheritWeight * grant.translation;
  }

  auto &base = pose.m_boneStates[index].inherited;
  if (!(pose.m_boneStates[index].physicsChannels & 1))
    base.rotation = value.rotation;
  if (!(pose.m_boneStates[index].physicsChannels & 2))
    base.translation = value.translation;

  pose.m_boneStates[index].dirty = true;
}

void PoseSolver::updateGlobalTransform(Pose &pose, uint32_t index) const {
  const auto parent = m_modelData->bones[index].parentIndex;
  const auto parentVersion =
      parent < 0 ? 0 : pose.m_boneStates[parent].globalVersion;

  if (!pose.m_boneStates[index].dirty &&
      pose.m_boneStates[index].parentVersion == parentVersion)
    return;

  Transform local = pose.m_boneStates[index].inherited;
  local.rotation = pose.m_boneStates[index].ikRotation * local.rotation;
  local.translation += m_boneLocalOffsets[index];

  auto &global = pose.m_globalBoneTransforms[index];
  const auto physics = global;
  global = parent < 0 ? local : local * pose.m_globalBoneTransforms[parent];

  const auto channels = pose.m_boneStates[index].physicsChannels;
  if (channels) {
    if (channels & 1)
      global.rotation = physics.rotation;
    if (channels & 2)
      global.translation = physics.translation;

    local = parent < 0 ? global
                       : global * pose.m_globalBoneTransforms[parent].inverse();
    local.translation -= m_boneLocalOffsets[index];
    local.rotation =
        glm::inverse(pose.m_boneStates[index].ikRotation) * local.rotation;
    pose.m_boneStates[index].inherited = local;
  }

  pose.m_boneStates[index].parentVersion = parentVersion;
  ++pose.m_boneStates[index].globalVersion;
  pose.m_boneStates[index].dirty = false;
}

void PoseSolver::ensureGlobalTransform(Pose &pose, uint32_t index) const {
  const auto parent = m_modelData->bones[index].parentIndex;
  if (parent >= 0)
    ensureGlobalTransform(pose, parent);

  updateGlobalTransform(pose, index);
}

void PoseSolver::updateGlobalTransforms(Pose &pose) const {
  for (const auto i : m_hierarchyOrder)
    updateGlobalTransform(pose, i);
}

#if !defined(GLMMD_DONT_USE_BULLET)
static btTransform glm2bt(const Transform &t) {
  return btTransform(
      btQuaternion(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w),
      btVector3(t.translation.x, t.translation.y, t.translation.z));
}

static Transform bt2glm(const btTransform &t) {
  const auto &o = t.getOrigin();
  const auto q = t.getRotation();
  return {.translation = glm::vec3(o.x(), o.y(), o.z()),
          .rotation = glm::quat(q.w(), q.x(), q.y(), q.z())};
}
#endif

void PoseSolver::syncStaticRigidBodyTransforms(const Pose &pose,
                                               const RigidBodyData &rb,
                                               int32_t bi) const {
#if !defined(GLMMD_DONT_USE_BULLET)
  Transform t = rb.offset;
  t.translation -= m_modelData->bones[bi].position;
  rb.motionState->setWorldTransform(
      glm2bt(t * pose.m_globalBoneTransforms[bi]));
#endif
}

void PoseSolver::syncDynamicRigidBodyTransforms(Pose &pose,
                                                const RigidBodyData &rb,
                                                int32_t bi) const {
#if !defined(GLMMD_DONT_USE_BULLET)
  btTransform transform;
  rb.motionState->getWorldTransform(transform);

  Transform offset = rb.offset;
  offset.translation -= m_modelData->bones[bi].position;

  pose.m_globalBoneTransforms[bi] = offset.inverse() * bt2glm(transform);
  pose.m_boneStates[bi].physicsChannels = 3;
#endif
}

void PoseSolver::syncMixedRigidBodyTransforms(Pose &pose,
                                              const RigidBodyData &rb,
                                              int32_t bi) const {
#if !defined(GLMMD_DONT_USE_BULLET)
  btTransform transform;
  rb.motionState->getWorldTransform(transform);

  pose.m_globalBoneTransforms[bi].rotation =
      bt2glm(transform).rotation * glm::inverse(rb.offset.rotation);
  pose.m_boneStates[bi].physicsChannels = 1;
#endif
}

void PoseSolver::syncWithPhysics(Pose &pose, ModelPhysics &physics) const {
  const auto &bodies = physics.m_impl->rigidBodies;

  // Send all kinematic transforms before replacing any globals with readback.
  for (size_t i = 0; i < bodies.size(); ++i) {
    const auto &data = m_modelData->rigidBodies[i];
    if (data.boneIndex >= 0 && data.physicsCalcType == PhysicsCalcType::Static)
      syncStaticRigidBodyTransforms(pose, bodies[i], data.boneIndex);
  }

  // Read all authoritative globals before reconciling the hierarchy. Body array
  // order need not match parent order, and driven children keep their own
  // globals.
  for (size_t i = 0; i < bodies.size(); ++i) {
    const auto &data = m_modelData->rigidBodies[i];
    const auto bi = data.boneIndex;
    if (bi < 0 || data.physicsCalcType == PhysicsCalcType::Static)
      continue;

    if (data.physicsCalcType == PhysicsCalcType::Dynamic)
      syncDynamicRigidBodyTransforms(pose, bodies[i], bi);
    else
      syncMixedRigidBodyTransforms(pose, bodies[i], bi);

    pose.m_boneStates[bi].ikRotation = glm::identity<glm::quat>();
    pose.m_boneStates[bi].dirty = true;
  }

  updateGlobalTransforms(pose);

#if !defined(GLMMD_DONT_USE_BULLET)
  for (size_t i = 0; i < bodies.size(); ++i) {
    const auto &data = m_modelData->rigidBodies[i];
    if (data.boneIndex < 0 || data.physicsCalcType != PhysicsCalcType::Mixed)
      continue;

    const auto &rb = bodies[i];
    btTransform transform;
    rb.motionState->getWorldTransform(transform);

    const auto translation =
        pose.m_globalBoneTransforms[data.boneIndex] *
        (rb.offset.translation - m_modelData->bones[data.boneIndex].position);
    transform.setOrigin(btVector3(translation.x, translation.y, translation.z));
    rb.motionState->setWorldTransform(transform);
  }
#endif
}

} // namespace glmmd
