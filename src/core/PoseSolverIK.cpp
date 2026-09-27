#include <glmmd/core/PoseSolver.h>

#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <cmath>

namespace glmmd {
namespace {

// GLM's quat(vec3) uses Rz * Ry * Rx. Choose an equivalent decomposition near
// the last accepted angles, rather than jumping branches at +/- pi or gimbal
// lock.
glm::vec3 continuousEuler(const glm::quat &rotation,
                          const glm::vec3 &previous) {
  const auto m = glm::mat3_cast(rotation);
  const float sy = glm::clamp(-m[0][2], -1.f, 1.f);

  glm::vec3 angles;
  angles.y = std::asin(sy);
  if (1.f - std::abs(sy) > 1e-6f) {
    angles.x = std::atan2(m[1][2], m[2][2]);
    angles.z = std::atan2(m[0][1], m[0][0]);
  } else {
    angles.x = previous.x;
    angles.z =
        std::atan2(-m[1][0], m[1][1]) + (sy >= 0.f ? angles.x : -angles.x);
  }

  auto unwrap = [&](glm::vec3 value) {
    for (int axis = 0; axis < 3; ++axis)
      value[axis] =
          previous[axis] +
          std::remainder(value[axis] - previous[axis], glm::two_pi<float>());
    return value;
  };

  const auto alternate =
      unwrap(glm::vec3(angles.x + glm::pi<float>(), glm::pi<float>() - angles.y,
                       angles.z + glm::pi<float>()));
  angles = unwrap(angles);

  return glm::distance2(alternate, previous) < glm::distance2(angles, previous)
             ? alternate
             : angles;
}

glm::vec3 perpendicular(const glm::vec3 &v) {
  const auto a = glm::abs(v);
  glm::vec3 basis(0.f);
  basis[a.x < a.y ? (a.x < a.z ? 0 : 2) : (a.y < a.z ? 1 : 2)] = 1.f;

  return glm::normalize(glm::cross(v, basis));
}

} // namespace

void PoseSolver::solveIK(Pose &pose, uint32_t ikIndex) const {
  const auto &ik = m_modelData->ikData[ikIndex];
  if (ik.endEffector < 0 || ik.targetBoneIndex < 0 || ik.links.empty() ||
      ik.loopCount <= 0 || ik.limitAngle <= 0.f)
    return;

  const auto &plan = m_ikPlans[ikIndex];
  ensureGlobalTransform(pose, ik.targetBoneIndex);

  const auto goal = pose.m_globalBoneTransforms[ik.targetBoneIndex].translation;
  const float tolerance2 = plan.positionTolerance * plan.positionTolerance;
  const float maxStep = std::min(ik.limitAngle, glm::pi<float>());

  auto refreshEffector = [&] {
    for (auto index : plan.effectorPath)
      updateGlobalTransform(pose, index);

    return pose.m_globalBoneTransforms[ik.endEffector].translation;
  };

  auto setRotation = [&](uint32_t bone, glm::quat rotation) {
    auto &delta = pose.m_boneStates[bone].ikRotation;
    rotation = glm::normalize(
        rotation * glm::inverse(pose.m_boneStates[bone].inherited.rotation));

    if (glm::dot(rotation, delta) < 0.f)
      rotation = -rotation;
    if (glm::length2(rotation - delta) < 1e-14f)
      return false;

    delta = rotation;
    pose.m_boneStates[bone].dirty = true;
    return true;
  };

  // Project the initial composed rotation into its limits, including fixed
  // links. Overlapping controllers start from the current IK result, not
  // identity.
  for (size_t j = 0; j < ik.links.size(); ++j) {
    const auto &link = ik.links[j];
    const auto bi = link.boneIndex;
    auto &angles = pose.m_ikStates[plan.scratchOffset + j].previousAngles;
    const auto rotation = pose.m_boneStates[bi].ikRotation *
                          pose.m_boneStates[bi].inherited.rotation;

    angles = continuousEuler(
        rotation,
        link.angleLimitFlag
            ? glm::clamp(glm::vec3(0.f), link.lowerLimit, link.upperLimit)
            : glm::vec3(0.f));

    if (link.angleLimitFlag && !(pose.m_boneStates[bi].physicsChannels & 1)) {
      angles = glm::clamp(angles, link.lowerLimit, link.upperLimit);
      setRotation(bi, glm::quat(angles));
    }

    pose.m_ikStates[plan.scratchOffset + j].bestRotation =
        pose.m_boneStates[bi].ikRotation;
  }

  float bestError = glm::distance2(refreshEffector(), goal);
  int stagnantSweeps = 0;

  for (int32_t iteration = 0;
       iteration < ik.loopCount && bestError > tolerance2; ++iteration) {
    bool changed = false;
    for (size_t j = 0; j < ik.links.size(); ++j) {
      const auto &link = ik.links[j];
      const auto &lp = plan.links[j];
      const auto bi = link.boneIndex;
      if (lp.fixed || (pose.m_boneStates[bi].physicsChannels & 1))
        continue;

      const auto effector = refreshEffector();
      if (glm::distance2(effector, goal) <= tolerance2)
        break;

      for (auto index : lp.parentPath)
        updateGlobalTransform(pose, index);
      updateGlobalTransform(pose, bi);

      const auto parent = m_modelData->bones[bi].parentIndex;
      const auto inverseParent =
          parent < 0
              ? glm::identity<glm::quat>()
              : glm::inverse(pose.m_globalBoneTransforms[parent].rotation);

      const auto pivot = pose.m_globalBoneTransforms[bi].translation;
      auto from = inverseParent * (effector - pivot);
      auto to = inverseParent * (goal - pivot);
      const float from2 = glm::length2(from), to2 = glm::length2(to);
      if (from2 <= tolerance2 * 1e-4f || to2 <= tolerance2 * 1e-4f)
        continue;

      const float fromLength = std::sqrt(from2), toLength = std::sqrt(to2);
      from /= fromLength;
      to /= toLength;

      auto &previous = pose.m_ikStates[plan.scratchOffset + j].previousAngles;
      const auto current = pose.m_boneStates[bi].ikRotation *
                           pose.m_boneStates[bi].inherited.rotation;

      glm::quat next;
      if (lp.hingeAxis >= 0) {
        const int axis = lp.hingeAxis;
        from[axis] = to[axis] = 0.f;
        const float projectedFrom2 = glm::length2(from),
                    projectedTo2 = glm::length2(to);
        if (projectedFrom2 < 1e-12f || projectedTo2 < 1e-12f)
          continue;

        from /= std::sqrt(projectedFrom2);
        to /= std::sqrt(projectedTo2);

        float sine = glm::cross(from, to)[axis];
        const float cosine = glm::clamp(glm::dot(from, to), -1.f, 1.f);
        float angle = std::atan2(sine, cosine);

        // Exactly straight chains have no preferred bend direction. Choose one
        // permitted by the hinge, and seed a small bend when the goal is
        // nearer.
        if (std::abs(sine) < 1e-7f) {
          const float positiveRoom = link.upperLimit[axis] - previous[axis];
          const float negativeRoom = previous[axis] - link.lowerLimit[axis];
          const float sign = positiveRoom >= negativeRoom ? 1.f : -1.f;
          if (cosine < 0.f)
            angle = sign * glm::pi<float>();
          else if (iteration == 0 && ik.links.size() > 1 &&
                   toLength + plan.positionTolerance < fromLength)
            angle = sign * std::min(0.05f, maxStep);
        }

        auto angles = previous;
        angles[axis] =
            glm::clamp(previous[axis] + glm::clamp(angle, -maxStep, maxStep),
                       link.lowerLimit[axis], link.upperLimit[axis]);
        previous = angles;
        next = glm::quat(angles);
      } else {
        auto axis = glm::cross(from, to);
        const float sine = glm::length(axis);
        const float cosine = glm::clamp(glm::dot(from, to), -1.f, 1.f);
        float angle = std::atan2(sine, cosine);

        if (sine < 1e-7f) {
          if (cosine >= 0.f) {
            if (iteration != 0 || ik.links.size() < 2 ||
                toLength + plan.positionTolerance >= fromLength)
              continue;
            angle = std::min(0.05f, maxStep);
          }

          axis = perpendicular(from);
        } else {
          axis /= sine;
        }

        next = glm::normalize(glm::angleAxis(std::min(angle, maxStep), axis) *
                              current);

        if (link.angleLimitFlag) {
          auto angles = continuousEuler(next, previous);
          angles = glm::clamp(angles, link.lowerLimit, link.upperLimit);

          // Limit each Euler correction too: projection near a singularity may
          // otherwise turn a small CCD step into a large constrained rotation.
          angles = previous + glm::clamp(angles - previous, glm::vec3(-maxStep),
                                         glm::vec3(maxStep));
          previous = angles;
          next = glm::quat(angles);
        }
      }

      changed |= setRotation(bi, next);
    }

    const float error = glm::distance2(refreshEffector(), goal);
    if (error < bestError) {
      bestError = error;
      stagnantSweeps = 0;
      for (size_t j = 0; j < ik.links.size(); ++j)
        pose.m_ikStates[plan.scratchOffset + j].bestRotation =
            pose.m_boneStates[ik.links[j].boneIndex].ikRotation;
    } else {
      ++stagnantSweeps;
    }

    if (!changed || (iteration >= 3 && stagnantSweeps >= 4))
      break;
  }

  // Constraints can worsen a later sweep. Keep the best accepted result and let
  // lazy FK refresh affected descendants before any subsequent consumer reads
  // them.
  for (size_t j = 0; j < ik.links.size(); ++j) {
    const auto bi = ik.links[j].boneIndex;
    const auto best = pose.m_ikStates[plan.scratchOffset + j].bestRotation;
    if (pose.m_boneStates[bi].ikRotation != best) {
      pose.m_boneStates[bi].ikRotation = best;
      pose.m_boneStates[bi].dirty = true;
    }
  }
}

} // namespace glmmd
