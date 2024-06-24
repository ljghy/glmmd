#include <algorithm>
#include <numeric>

#include <glmmd/core/ModelPoseSolver.h>

namespace glmmd
{

ModelPoseSolver::ModelPoseSolver(
    const std::shared_ptr<const ModelData> &modelData)
    : m_modelData(modelData)
    , m_boneChildren(modelData->bones.size())
    , m_boneDeformOrder(modelData->bones.size())
{
    for (uint32_t i = 0; i < modelData->bones.size(); ++i)
    {
        const auto &bone = modelData->bones[i];
        if (bone.parentIndex != -1)
            m_boneChildren[bone.parentIndex].push_back(i);
    }

    sortBoneDeformOrder();
}

void ModelPoseSolver::sortBoneDeformOrder()
{
    std::iota(m_boneDeformOrder.begin(), m_boneDeformOrder.end(), 0);
    std::stable_sort(
        m_boneDeformOrder.begin(), m_boneDeformOrder.end(),
        [&](uint32_t a, uint32_t b)
        {
            const auto &boneA = m_modelData->bones[a];
            const auto &boneB = m_modelData->bones[b];
            if (!boneA.deformAfterPhysics() && boneB.deformAfterPhysics())
                return true;
            if (boneA.deformAfterPhysics() && !boneB.deformAfterPhysics())
                return false;
            if (boneA.deformLayer != boneB.deformLayer)
                return boneA.deformLayer < boneB.deformLayer;
            return a < b;
        });

    auto currentLayer = m_modelData->bones[m_boneDeformOrder[0]].deformLayer;
    uint32_t offset   = 0;
    auto     it       = m_boneDeformOrder.begin();
    while (it != m_boneDeformOrder.end())
    {
        it = std::find_if(
            m_boneDeformOrder.begin() + offset, m_boneDeformOrder.end(),
            [&](uint32_t i)
            {
                return m_modelData->bones[i].deformLayer != currentLayer ||
                       m_modelData->bones[i].deformAfterPhysics();
            });
        auto last =
            static_cast<uint32_t>(std::distance(m_boneDeformOrder.begin(), it));
        m_updateBeforePhysicsRanges.emplace_back(offset, last);
        offset = last;
        if (it != m_boneDeformOrder.end())
        {
            currentLayer = m_modelData->bones[*it].deformLayer;
            if (m_modelData->bones[*it].deformAfterPhysics())
                break;
        }
    }
    while (it != m_boneDeformOrder.end())
    {
        it = std::find_if(
            m_boneDeformOrder.begin() + offset, m_boneDeformOrder.end(),
            [&](uint32_t i)
            { return m_modelData->bones[i].deformLayer != currentLayer; });
        auto last =
            static_cast<uint32_t>(std::distance(m_boneDeformOrder.begin(), it));
        m_updateAfterPhysicsRanges.emplace_back(offset, last);
        offset = last;
        if (it != m_boneDeformOrder.end())
            currentLayer = m_modelData->bones[*it].deformLayer;
    }
}

void ModelPoseSolver::solveBeforePhysics(ModelPose &pose) const
{
    applyGroupMorphs(pose);
    applyBoneMorphs(pose);

    for (const auto &[first, last] : m_updateBeforePhysicsRanges)
    {
        solveGlobalBoneTransforms(pose, first, last);
        solveIK(pose, first, last);
        updateInheritedBoneTransforms(pose, first, last);
        solveGlobalBoneTransforms(pose, first, last);
    }
}

void ModelPoseSolver::solveAfterPhysics(ModelPose &pose) const
{
    for (const auto &[first, last] : m_updateAfterPhysicsRanges)
    {
        solveGlobalBoneTransforms(pose, first, last);
        solveIK(pose, first, last);
        updateInheritedBoneTransforms(pose, first, last);
        solveGlobalBoneTransforms(pose, first, last);
    }
}

static btTransform glm2bt(const Transform &t)
{
    btTransform transform;
    transform.setRotation(
        btQuaternion(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w));
    transform.setOrigin(
        btVector3(t.translation.x, t.translation.y, t.translation.z));
    return transform;
}

static Transform bt2glm(const btTransform &t)
{
    auto o = t.getOrigin();
    auto q = t.getRotation();

    Transform transform{glm::vec3(o.x(), o.y(), o.z()),
                        glm::quat(q.w(), q.x(), q.y(), q.z())};
    return transform;
}

void ModelPoseSolver::syncStaticRigidBodyTransforms(const ModelPose     &pose,
                                                    const RigidBodyData &rb,
                                                    int32_t bi) const
{
    Transform t = rb.offset;
    t.translation -= m_modelData->bones[bi].position;
    t *= pose.m_globalBoneTransforms[bi];
    rb.motionState->setWorldTransform(glm2bt(t));
}

void ModelPoseSolver::syncDynamicRigidBodyTransforms(ModelPose           &pose,
                                                     const RigidBodyData &rb,
                                                     int32_t bi) const
{
    btTransform transform;
    rb.motionState->getWorldTransform(transform);
    Transform t = rb.offset;
    t.translation -= m_modelData->bones[bi].position;
    pose.m_globalBoneTransforms[bi] = t.inverse() * bt2glm(transform);

    for (auto k : m_boneChildren[bi])
        solveChildGlobalBoneTransforms(pose, k);
}

void ModelPoseSolver::syncMixedRigidBodyTransforms(ModelPose           &pose,
                                                   const RigidBodyData &rb,
                                                   int32_t bi) const
{
    btTransform transform;
    rb.motionState->getWorldTransform(transform);

    auto      q        = transform.getRotation();
    glm::quat rotation = glm::quat(q.w(), q.x(), q.y(), q.z()) *
                         glm::inverse(rb.offset.rotation);
    pose.m_globalBoneTransforms[bi].rotation = rotation;

    glm::vec3 translation =
        pose.m_globalBoneTransforms[bi] *
        (rb.offset.translation - m_modelData->bones[bi].position);

    transform.setOrigin(btVector3(translation.x, translation.y, translation.z));
    rb.motionState->setWorldTransform(transform);

    for (auto k : m_boneChildren[bi])
        solveChildGlobalBoneTransforms(pose, k);
}

void ModelPoseSolver::syncWithPhysics(ModelPose    &pose,
                                      ModelPhysics &physics) const
{
    for (size_t i = 0; i < physics.rigidBodies.size(); ++i)
    {
        auto   &rb = physics.rigidBodies[i];
        int32_t j  = m_modelData->rigidBodies[i].boneIndex;
        if (j < 0)
            continue;

        switch (m_modelData->rigidBodies[i].physicsCalcType)
        {
        case PhysicsCalcType::Static:
            syncStaticRigidBodyTransforms(pose, rb, j);
            break;
        case PhysicsCalcType::Dynamic:
            syncDynamicRigidBodyTransforms(pose, rb, j);
            break;
        case PhysicsCalcType::Mixed:
            syncMixedRigidBodyTransforms(pose, rb, j);
            break;
        }
    }
}

void ModelPoseSolver::applyGroupMorphs(ModelPose &pose) const
{
    for (uint32_t i = 0; i < pose.m_morphRatios.size(); ++i)
    {
        const auto &morph = m_modelData->morphs[i];
        if (pose.m_morphRatios[i] == 0.f || morph.type != MorphType::Group)
            continue;

        for (int32_t j = 0; j < morph.count; ++j)
        {
            const auto &m = morph.group[j];
            if (m_modelData->morphs[m.index].type != MorphType::Group)
                pose.m_morphRatios[m.index] += pose.m_morphRatios[i] * m.ratio;
        }
    }
}

void ModelPoseSolver::applyBoneMorphs(ModelPose &pose) const
{
    for (uint32_t i = 0; i < pose.m_morphRatios.size(); ++i)
    {
        const auto &morph = m_modelData->morphs[i];
        if (morph.type == MorphType::Bone && pose.m_morphRatios[i] != 0.f)
        {
            for (int32_t j = 0; j < morph.count; ++j)
            {
                const auto &boneMorph = morph.bone[j];
                pose.m_localBoneTransforms[boneMorph.index] *=
                    pose.m_morphRatios[i] *
                    Transform{boneMorph.translation, boneMorph.rotation};
            }
        }
    }
}

void ModelPoseSolver::solveIK(ModelPose &pose, uint32_t first,
                              uint32_t last) const
{
    for (; first != last; ++first)
    {
        const auto &bone = m_modelData->bones[m_boneDeformOrder[first]];
        if (!bone.isIK())
            continue;
        const auto &ik = m_modelData->ikData[bone.ikDataIndex];

        glm::vec3 targetPos =
            pose.getGlobalBonePosition(ik.realTargetBoneIndex);

        for (int32_t i = 0; i < ik.loopCount; ++i)
        {
            bool converged = false;

            for (const auto &link : ik.links)
            {
                if (link.angleLimitFlag && i == 0 && ik.loopCount > 1)
                {
                    pose.m_localBoneTransforms[link.boneIndex].rotation =
                        glm::quat(0.5f * (link.lowerLimit + link.upperLimit));
                    solveChildGlobalBoneTransforms(pose, link.boneIndex,
                                                   ik.targetBoneIndex);
                    continue;
                }

                glm::vec3 endEffectorPos =
                    pose.getGlobalBonePosition(ik.targetBoneIndex);

                constexpr float tol = 1e-5f;

                if (glm::distance(endEffectorPos, targetPos) < tol)
                {
                    converged = true;
                    break;
                }

                glm::vec3 linkPos = pose.getGlobalBonePosition(link.boneIndex);

                glm::vec3 linkToTarget      = targetPos - linkPos;
                glm::vec3 linkToEndEffector = endEffectorPos - linkPos;

                glm::vec3 axis = glm::cross(linkToEndEffector, linkToTarget);
                float     axisLength = glm::length(axis);
                if (axisLength < tol)
                    continue;

                axis /= axisLength;

                int32_t parentIndex =
                    m_modelData->bones[link.boneIndex].parentIndex;
                glm::mat3 localAxes =
                    parentIndex == -1
                        ? glm::mat3(1.f)
                        : glm::transpose(glm::mat3_cast(
                              pose.m_globalBoneTransforms[parentIndex]
                                  .rotation));

                glm::vec3 localAxis = localAxes * axis;

                float angle = glm::clamp(
                    glm::atan(axisLength,
                              glm::dot(linkToTarget, linkToEndEffector)),
                    -ik.limitAngle, ik.limitAngle);

                auto &rot = pose.m_localBoneTransforms[link.boneIndex].rotation;
                rot = glm::normalize(glm::angleAxis(angle, localAxis) * rot);

                if (link.angleLimitFlag)
                {
                    glm::vec3 euler = glm::eulerAngles(rot);
                    euler = glm::clamp(euler, link.lowerLimit, link.upperLimit);
                    rot   = glm::quat(euler);
                }

                solveChildGlobalBoneTransforms(pose, link.boneIndex,
                                               ik.targetBoneIndex);
            }

            if (converged)
                break;
        }
    }
}

bool ModelPoseSolver::solveChildGlobalBoneTransforms(ModelPose &pose,
                                                     uint32_t   boneIndex,
                                                     int32_t    stop) const
{
    const auto &bone                   = m_modelData->bones[boneIndex];
    glm::vec3   localTranslationOffset = bone.position;
    if (bone.parentIndex != -1)
        localTranslationOffset -= m_modelData->bones[bone.parentIndex].position;
    Transform localTransform = pose.m_localBoneTransforms[boneIndex];
    localTransform.translation += localTranslationOffset;
    if (bone.parentIndex != -1)
        pose.m_globalBoneTransforms[boneIndex] =
            localTransform * pose.m_globalBoneTransforms[bone.parentIndex];
    else
        pose.m_globalBoneTransforms[boneIndex] = localTransform;

    if (boneIndex == static_cast<uint32_t>(stop))
        return false;

    for (auto i : m_boneChildren[boneIndex])
        if (!solveChildGlobalBoneTransforms(pose, i, stop))
            return false;
    return true;
}

void ModelPoseSolver::solveGlobalBoneTransforms(ModelPose &pose, uint32_t first,
                                                uint32_t last) const
{
    for (; first < last; ++first)
    {
        uint32_t i = m_boneDeformOrder[first];

        const auto &bone = m_modelData->bones[i];

        glm::vec3 localTranslationOffset = bone.position;
        if (bone.parentIndex != -1)
            localTranslationOffset -=
                m_modelData->bones[bone.parentIndex].position;

        Transform localTransform = pose.m_localBoneTransforms[i];
        localTransform.translation += localTranslationOffset;
        if (bone.parentIndex != -1)
            pose.m_globalBoneTransforms[i] =
                localTransform * pose.m_globalBoneTransforms[bone.parentIndex];
        else
            pose.m_globalBoneTransforms[i] = localTransform;
    }
}

void ModelPoseSolver::updateInheritedBoneTransforms(ModelPose &pose,
                                                    uint32_t   first,
                                                    uint32_t   last) const
{
    for (; first != last; ++first)
    {
        uint32_t    i    = m_boneDeformOrder[first];
        const auto &bone = m_modelData->bones[i];

        Transform inheritedTransform = identityTransform;

        if (bone.inheritRotation() && bone.inheritParentIndex != -1)
            inheritedTransform.rotation = glm::slerp(
                inheritedTransform.rotation,
                pose.m_localBoneTransforms[bone.inheritParentIndex].rotation,
                bone.inheritWeight);
        if (bone.inheritTranslation() && bone.inheritParentIndex != -1)
            inheritedTransform.translation =
                bone.inheritWeight *
                pose.m_localBoneTransforms[bone.inheritParentIndex].translation;

        pose.m_localBoneTransforms[i] *= inheritedTransform;
    }
}

} // namespace glmmd
