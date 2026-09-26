#ifndef GLMMD_CORE_INTERPOLATION_CURVE_H_
#define GLMMD_CORE_INTERPOLATION_CURVE_H_

#include <array>

namespace glmmd {

// 2D Bézier curve with control points {(0, 0), (x1, y1), (x2, y2), (1, 1)}
using InterpolationCurveNodes = std::array<float, 4>; // {x1, y1, x2, y2}

float evalCurve(const InterpolationCurveNodes &curve, float x);

} // namespace glmmd

#endif
