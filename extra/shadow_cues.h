#pragma once

#include <algorithm>
#include <cmath>

namespace extra {

inline float shadowPulse(unsigned time, unsigned center, float halfWidth)
{
    const float phase = std::max(0.f, 1.f - std::abs(float(time) - float(center)) / halfWidth);
    return phase * phase;
}

inline float shadowNegativePulse(unsigned time, float halfWidth)
{
    // Match the two negative intervals in ParticleScene.cpp.
    return std::max({shadowPulse(time, 30000, halfWidth),
        shadowPulse(time, 40000, halfWidth), shadowPulse(time, 44700, halfWidth),
        shadowPulse(time, 49900, halfWidth)});
}

inline float shadowCutBlur(unsigned time)
{
    return std::max(shadowPulse(time, 7700, 1150.f),
        shadowPulse(time, 29400, 1000.f));
}

}
