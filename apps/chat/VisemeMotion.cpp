#include "VisemeMotion.h"

static const std::string visemeMap[]{
    "eyeBlinkLeft",      "eyeLookDownLeft",    "eyeLookInLeft",
    "eyeLookOutLeft",    "eyeLookUpLeft",      "eyeSquintLeft",
    "eyeWideLeft",       "eyeBlinkRight",      "eyeLookDownRight",
    "eyeLookInRight",    "eyeLookOutRight",    "eyeLookUpRight",
    "eyeSquintRight",    "eyeWideRight",       "jawForward",
    "jawLeft",           "jawRight",           "jawOpen",
    "mouthClose",        "mouthFunnel",        "mouthPucker",
    "mouthLeft",         "mouthRight",         "mouthSmileLeft",
    "mouthSmileRight",   "mouthFrownLeft",     "mouthFrownRight",
    "mouthDimpleLeft",   "mouthDimpleRight",   "mouthStretchLeft",
    "mouthStretchRight", "mouthRollLower",     "mouthRollUpper",
    "mouthShrugLower",   "mouthShrugUpper",    "mouthPressLeft",
    "mouthPressRight",   "mouthLowerDownLeft", "mouthLowerDownRight",
    "mouthUpperUpLeft",  "mouthUpperUpRight",  "browDownLeft",
    "browDownRight",     "browInnerUp",        "browOuterUpLeft",
    "browOuterUpRight",  "cheekPuff",          "cheekSquintLeft",
    "cheekSquintRight",  "noseSneerLeft",      "noseSneerRight",
    "tongueOut",         "headRoll",           "leftEyeRoll",
    "rightEyeRoll"};

VisemeMotion::VisemeMotion(const glmmd::ModelData    &data,
                           const std::vector<Viseme> &visemes)
    : m_visemes(visemes)
{
    for (size_t i = 0; i < nVisemeTypes; ++i)
    {
        m_morphMap[i] = data.getMorphIndex(visemeMap[i]);
    }
}

void VisemeMotion::getLocalPose(float time, glmmd::ModelPose &pose) const
{
    size_t frame = static_cast<size_t>(time * VisemeFrameRate + 0.5f);

    if (frame >= m_visemes.size())
        return;

    for (size_t i = 0; i < nVisemeTypes; ++i)
    {
        float ratio = m_visemes[frame][i];
        if (ratio != 0.f)
        {
            int32_t j = m_morphMap[i];
            if (j >= 0)
                pose.setMorphRatio(j, ratio);
        }
    }
}