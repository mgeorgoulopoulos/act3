#include "shadow_camera.h"
#include "mode.h"
#include "../compatibility/d3d9/indexed_draw.h"

void extra::applyShadowCamera(d3d9::Device& device, unsigned milliseconds,
    d3d9::Matrix& projection)
{
    constexpr unsigned start = 20000, end = 27000;
    if (!enabled || milliseconds < start || milliseconds >= end)
        return;

    const float phase = static_cast<float>(milliseconds - start) / (end - start);
    const float eased = phase * phase * (3 - 2 * phase);
    constexpr float farDistance = 650, nearDistance = 290;
    const float distance = farDistance + (nearDistance - farDistance) * eased;

    // Keep the figure between the camera and the central star.
    d3d9::Matrix view;
    view._42 = 5;
    view._43 = 300 + distance;
    device.SetTransform(d3d9::D3DTS_VIEW, &view);

    // Projected size at the focal plane is proportional to focal length / distance.
    const float zoom = distance / farDistance;
    projection._11 *= zoom;
    projection._22 *= zoom;
}
