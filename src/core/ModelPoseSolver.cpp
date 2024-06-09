#include <algorithm>
#include <queue>
#include <numeric>

#include <glmmd/core/ModelPoseSolver.h>

namespace glmmd
{

constexpr float IK_SOLVER_THRESHOLD = 1e-5f;

ModelPoseSolver::ModelPoseSolver(
    const std::shared_ptr<const ModelData> &modelData)
    : m_modelData(modelData)
    , m_boneChildren(modelData->bones.size())
    , m_boneDeformOrder(modelData->bones.size())
    , m_afterPhysicsStartIndex(static_cast<uint32_t>(modelData->bones.size()))
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

    m_afterPhysicsStartIndex = static_cast<uint32_t>(std::distance(
        m_boneDeformOrder.begin(),
        std::find_if(m_boneDeformOrder.begin(), m_boneDeformOrder.end(),
                     [&](uint32_t i)
                     { return m_modelData->bones[i].deformAfterPhysics(); })));
}

void ModelPoseSolver::solveBeforePhysics(ModelPose &pose) const
{
    applyGroupMorphs(pose);
    applyBoneMorphs(pose);

    solveGlobalBoneTransformsRange(pose, 0u, m_afterPhysicsStartIndex);
    solveIK(pose);
    updateInheritedBoneTransforms(pose);
    solveGlobalBoneTransformsRange(pose, 0u, m_afterPhysicsStartIndex);
}

void ModelPoseSolver::solveAfterPhysics(ModelPose &pose) const
{
    solveGlobalBoneTransformsRange(pose, m_afterPhysicsStartIndex,
                                   m_modelData->bones.size());
}

void ModelPoseSolver::syncStaticRigidBodyTransforms(const ModelPose     &pose,
                                                    const RigidBodyData &rb,
                                                    int32_t bi) const
{
    glm::mat4 m = pose.getFinalBoneTransform(bi) *
                  glm::translate(glm::mat4(1.f), rb.translationOffset) *
                  glm::mat4_cast(rb.rotationOffset);

    btTransform transform;
    transform.setFromOpenGLMatrix(&m[0][0]);

    rb.motionState->setWorldTransform(transform);
}

void ModelPoseSolver::syncDynamicRigidBodyTransforms(ModelPose           &pose,
                                                     const RigidBodyData &rb,
                                                     int32_t bi) const
{
    btTransform transform;
    rb.motionState->getWorldTransform(transform);

    glm::mat4 m;
    transform.getOpenGLMatrix(&m[0][0]);

    pose.setGlobalBoneTransform(
        bi, m * glm::mat4_cast(glm::inverse(rb.rotationOffset)) *
                glm::translate(glm::mat4(1.f), m_modelData->bones[bi].position -
                                                   rb.translationOffset));

    for (auto k : m_boneChildren[bi])
        solveChildGlobalBoneTransforms(pose, k);
}

void ModelPoseSolver::syncMixedRigidBodyTransforms(ModelPose           &pose,
                                                   const RigidBodyData &rb,
                                                   int32_t bi) const
{
    btTransform transform;
    rb.motionState->getWorldTransform(transform);

    glm::vec3 translation = pose.getGlobalBonePosition(bi);

    const auto &q = transform.getRotation();
    glm::quat   rotation =
        glm::quat(q.w(), q.x(), q.y(), q.z()) * glm::inverse(rb.rotationOffset);

    pose.setGlobalBoneTransform(bi,
                                glm::translate(glm::mat4(1.f), translation) *
                                    glm::mat4_cast(rotation));

    translation = glm::vec3(
        pose.getGlobalBoneTransform(bi) *
        glm::vec4(rb.translationOffset - m_modelData->bones[bi].position, 1.f));

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
                const auto &transform = morph.bone[j];
                pose.m_localBoneTranslations[transform.index] +=
                    pose.m_morphRatios[i] * transform.translation;
                pose.m_localBoneRotations[transform.index] =
                    glm::slerp(glm::quat(1.f, 0.f, 0.f, 0.f),
                               transform.rotation, pose.m_morphRatios[i]) *
                    pose.m_localBoneRotations[transform.index];
            }
        }
    }
}

void ModelPoseSolver::solveIK(ModelPose &pose) const
{
    for (const auto &ik : m_modelData->ikData)
    {
        glm::vec3 targetPos =
            pose.getGlobalBonePosition(ik.realTargetBoneIndex);

        for (int32_t i = 0; i < ik.loopCount; ++i)
        {
            bool converged = false;

            for (const auto &link : ik.links)
            {
                if (link.angleLimitFlag && i == 0 && ik.loopCount > 1)
                {
                    pose.m_localBoneRotations[link.boneIndex] =
                        glm::quat(0.5f * (link.lowerLimit + link.upperLimit));
                    solveChildGlobalBoneTransforms(pose, link.boneIndex);
                    continue;
                }

                glm::vec3 endEffectorPos =
                    pose.getGlobalBonePosition(ik.targetBoneIndex);

                if (glm::distance(endEffectorPos, targetPos) <
                    IK_SOLVER_THRESHOLD)
                {
                    converged = true;
                    break;
                }

                glm::vec3 linkPos = pose.getGlobalBonePosition(link.boneIndex);

                glm::vec3 linkToTarget      = targetPos - linkPos;
                glm::vec3 linkToEndEffector = endEffectorPos - linkPos;

                glm::vec3 axis = glm::cross(linkToEndEffector, linkToTarget);
                float     axisLength = glm::length(axis);
                if (axisLength < IK_SOLVER_THRESHOLD)
                    continue;

                axis /= axisLength;

                int32_t parentIndex =
                    m_modelData->bones[link.boneIndex].parentIndex;
                glm::mat3 localAxes =
                    parentIndex == -1
                        ? glm::mat3(1.f)
                        : glm::transpose(glm::mat3(
                              pose.m_globalBoneTransforms[parentIndex]));

                glm::vec3 localAxis = localAxes * axis;

                float angle = glm::clamp(
                    glm::atan(axisLength,
                              glm::dot(linkToTarget, linkToEndEffector)),
                    -ik.limitAngle, ik.limitAngle);

                auto &rot = pose.m_localBoneRotations[link.boneIndex];
                rot = glm::normalize(glm::angleAxis(angle, localAxis) * rot);

                if (link.angleLimitFlag)
                {
                    glm::vec3 euler = glm::eulerAngles(rot);
                    euler = glm::clamp(euler, link.lowerLimit, link.upperLimit);
                    rot   = glm::quat(euler);
                }

                solveChildGlobalBoneTransforms(pose, link.boneIndex);
            }

            if (converged)
                break;
        }
    }
}

void ModelPoseSolver::solveChildGlobalBoneTransforms(ModelPose &pose,
                                                     uint32_t   boneIndex) const
{
    std::queue<uint32_t> que;

    que.push(boneIndex);

    while (!que.empty())
    {
        uint32_t i = que.front();
        que.pop();

        const auto &bone = m_modelData->bones[i];

        glm::vec3 localTranslationOffset = bone.position;
        if (bone.parentIndex != -1)
            localTranslationOffset -=
                m_modelData->bones[bone.parentIndex].position;

        pose.m_globalBoneTransforms[i] =
            glm::translate(glm::mat4(1.f), pose.m_localBoneTranslations[i] +
                                               localTranslationOffset) *
            glm::mat4_cast(pose.m_localBoneRotations[i]);

        if (bone.parentIndex != -1)
            pose.m_globalBoneTransforms[i] =
                pose.m_globalBoneTransforms[bone.parentIndex] *
                pose.m_globalBoneTransforms[i];

        for (int32_t j : m_boneChildren[i])
            que.push(j);
    }
}

void ModelPoseSolver::solveGlobalBoneTransformsRange(ModelPose &pose,
                                                     uint32_t   start,
                                                     uint32_t   end) const
{
    for (uint32_t j = start; j < end; ++j)
    {
        uint32_t i = m_boneDeformOrder[j];

        const auto &bone = m_modelData->bones[i];

        glm::vec3 localTranslationOffset = bone.position;
        if (bone.parentIndex != -1)
            localTranslationOffset -=
                m_modelData->bones[bone.parentIndex].position;

        pose.m_globalBoneTransforms[i] =
            glm::translate(glm::mat4(1.f), pose.m_localBoneTranslations[i] +
                                               localTranslationOffset) *
            glm::mat4_cast(pose.m_localBoneRotations[i]);

        if (bone.parentIndex != -1)
            pose.m_globalBoneTransforms[i] =
                pose.m_globalBoneTransforms[bone.parentIndex] *
                pose.m_globalBoneTransforms[i];
    }
}

void ModelPoseSolver::updateInheritedBoneTransforms(ModelPose &pose) const
{
    for (uint32_t j = 0; j < m_modelData->bones.size(); ++j)
    {
        uint32_t    i    = m_boneDeformOrder[j];
        const auto &bone = m_modelData->bones[i];
        if (bone.inheritRotation() && bone.inheritParentIndex != -1)
            pose.m_localBoneRotations[i] =
                glm::slerp(glm::quat(1.f, 0.f, 0.f, 0.f),
                           pose.m_localBoneRotations[bone.inheritParentIndex],
                           bone.inheritWeight) *
                pose.m_localBoneRotations[i];
        if (bone.inheritTranslation() && bone.inheritParentIndex != -1)
            pose.m_localBoneTranslations[i] +=
                pose.m_localBoneTranslations[bone.inheritParentIndex] *
                bone.inheritWeight;
    }
}

} // namespace glmmd
