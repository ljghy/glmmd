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
    , m_boneDeformOrder(modelData->bones.size())
    , m_afterPhysicsStartIndex(static_cast<uint32_t>(modelData->bones.size()))
{
    sortBoneDeformOrder();
}

void ModelPoseSolver::sortBoneDeformOrder()
{
    std::iota(m_boneDeformOrder.begin(), m_boneDeformOrder.end(), 0);
    std::sort(m_boneDeformOrder.begin(), m_boneDeformOrder.end(),
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

void ModelPoseSolver::solveBeforePhysics(ModelPose &pose)
{
    applyGroupMorphs(pose);
    applyBoneMorphs(pose);

    solveGlobalBoneTransformsBeforePhysics(pose);
    solveIK(pose);
    updateInheritedBoneTransforms(pose);
    solveGlobalBoneTransformsBeforePhysics(pose);
}

void ModelPoseSolver::solveAfterPhysics(ModelPose &pose)
{
    solveGlobalBoneTransformsAfterPhysics(pose);
}

void ModelPoseSolver::syncWithPhysics(ModelPose &pose, ModelPhysics &physics)
{
    for (size_t i = 0; i < physics.rigidBodies.size(); ++i)
    {
        auto   &rigidBody = physics.rigidBodies[i];
        int32_t j         = m_modelData->rigidBodies[i].boneIndex;
        if (j < 0)
            continue;

        if (m_modelData->rigidBodies[i].physicsCalcType ==
            PhysicsCalcType::Static)
        {
            glm::vec3 translation =
                glm::vec3(pose.getGlobalBoneTransform(j) *
                          glm::vec4(rigidBody.translationOffset -
                                        m_modelData->bones[j].position,
                                    1.f));

            glm::quat rotation =
                glm::quat_cast(pose.getGlobalBoneTransform(j)) *
                rigidBody.rotationOffset;

            btTransform transform;
            transform.setOrigin(
                btVector3(translation.x, translation.y, translation.z));
            transform.setRotation(
                btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));

            rigidBody.rigidBody->getMotionState()->setWorldTransform(transform);
        }
        else if (m_modelData->rigidBodies[i].physicsCalcType ==
                 PhysicsCalcType::Dynamic)
        {
            btTransform transform;
            rigidBody.rigidBody->getMotionState()->getWorldTransform(transform);

            glm::vec3 translation(transform.getOrigin().x(),
                                  transform.getOrigin().y(),
                                  transform.getOrigin().z());

            const auto &q        = transform.getRotation();
            glm::quat   rotation = glm::quat(q.w(), q.x(), q.y(), q.z()) *
                                 glm::inverse(rigidBody.rotationOffset);

            pose.setGlobalBoneTransform(
                j, glm::translate(glm::mat4(1.f), translation) *
                       glm::mat4_cast(rotation) *
                       glm::translate(glm::mat4(1.f),
                                      m_modelData->bones[j].position -
                                          rigidBody.translationOffset));
        }
        else // PhysicsCalcType::Mixed
        {
            // TODO:
            btTransform transform;
            rigidBody.rigidBody->getMotionState()->getWorldTransform(transform);

            glm::vec3 translation(transform.getOrigin().x(),
                                  transform.getOrigin().y(),
                                  transform.getOrigin().z());

            const auto &q        = transform.getRotation();
            glm::quat   rotation = glm::quat(q.w(), q.x(), q.y(), q.z()) *
                                 glm::inverse(rigidBody.rotationOffset);

            pose.setGlobalBoneTransform(
                j, glm::translate(glm::mat4(1.f), translation) *
                       glm::mat4_cast(rotation) *
                       glm::translate(glm::mat4(1.f),
                                      m_modelData->bones[j].position -
                                          rigidBody.translationOffset));
        }
    }
}

void ModelPoseSolver::applyGroupMorphs(ModelPose &pose)
{
    for (uint32_t i = 0; i < pose.m_morphRatios.size(); ++i)
    {
        if (pose.m_morphRatios[i] == 0.f ||
            m_modelData->morphs[i].type != MorphType::Group)
            continue;

        for (const auto &data : m_modelData->morphs[i].data)
            if (m_modelData->morphs[data.group.index].type != MorphType::Group)
                pose.m_morphRatios[data.group.index] +=
                    pose.m_morphRatios[i] * data.group.ratio;
    }
}

void ModelPoseSolver::applyBoneMorphs(ModelPose &pose)
{
    for (uint32_t i = 0; i < pose.m_morphRatios.size(); ++i)
    {
        if (m_modelData->morphs[i].type == MorphType::Bone &&
            pose.m_morphRatios[i] != 0.f)
        {
            for (const auto &data : m_modelData->morphs[i].data)
            {
                const auto &transform = data.bone;
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

void ModelPoseSolver::solveIK(ModelPose &pose)
{
    for (const auto &ik : m_modelData->ikData)
    {
        glm::vec3 targetPos =
            pose.getBoneGlobalPosition(ik.realTargetBoneIndex);

        for (int32_t i = 0; i < ik.loopCount; ++i)
        {
            bool converged = false;

            for (const auto &link : ik.links)
            {
                glm::vec3 endEffectorPos =
                    pose.getBoneGlobalPosition(ik.targetBoneIndex);

                if (glm::distance(endEffectorPos, targetPos) <
                    IK_SOLVER_THRESHOLD)
                {
                    converged = true;
                    break;
                }

                glm::vec3 linkPos = pose.getBoneGlobalPosition(link.boneIndex);

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
                    glm::asin(glm::clamp(axisLength /
                                             (glm::length(linkToEndEffector) *
                                              glm::length(linkToTarget)),
                                         -1.f, 1.f)),
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
                                                     uint32_t   boneIndex)
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

        for (int32_t j : bone.children)
            que.push(j);
    }
}

void ModelPoseSolver::solveGlobalBoneTransformsBeforePhysics(ModelPose &pose)
{
    for (uint32_t j = 0; j < m_afterPhysicsStartIndex; ++j)
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

void ModelPoseSolver::solveGlobalBoneTransformsAfterPhysics(ModelPose &pose)
{
    for (uint32_t j = m_afterPhysicsStartIndex; j < m_modelData->bones.size();
         ++j)
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

void ModelPoseSolver::updateInheritedBoneTransforms(ModelPose &pose)
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
