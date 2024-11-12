#include <cmath>
#include <limits>

#include <glmmd/core/InterpolationCurve.h>

namespace glmmd
{

float evalCurve(const InterpolationCurvePoints &curve, float x)
{
    const float x1 = curve[0], y1 = curve[1], x2 = curve[2], y2 = curve[3];

    float t = x, a = 3.f * (x1 - x2) + 1.f, b = 2.f * x2 - 4.f * x1;
    float t2, s, f, df;
    for (int iter = 0; iter < 8; ++iter)
    {
        t2 = t * t;
        s  = 1.f - t;
        df = a * t2 + b * t + x1;

        f = s * t * (s * x1 + t * x2) + (t2 * t - x) / 3.f;

        const float delta = f / (df + 1e-2f);
        t -= delta;
        if (std::abs(delta) < 1e-4f)
            break;
    }
    return 3.f * s * t * (s * y1 + t * y2) + t2 * t;
}

} // namespace glmmd
