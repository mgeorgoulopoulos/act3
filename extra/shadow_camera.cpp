#include "shadow_camera.h"
#include "mode.h"
#include "shadow_cues.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <algorithm>
#include <cmath>
namespace { float closeup; }
float extra::shadowCloseupStrength() { return closeup; }

float extra::shadowDollyDistance(unsigned milliseconds)
{
    constexpr unsigned start = 20000, end = 27000;
    if (milliseconds < start || milliseconds >= end)
        return 0;
    const float phase = static_cast<float>(milliseconds - start) / (end - start);
    const float eased = phase * phase * (3 - 2 * phase);
    constexpr float farDistance = 650, nearDistance = 290;
    return farDistance + (nearDistance - farDistance) * eased;
}

void extra::applyShadowCamera(d3d9::Device& device, unsigned milliseconds,
    d3d9::Matrix& projection)
{
    const float distance = shadowDollyDistance(milliseconds);
    if (!enabled)
        return;

    const float kick = 1.f - 0.38f * shadowNegativePulse(milliseconds, 220.f);
    projection._11 *= kick;
    projection._22 *= kick;

    if (distance) {
        // Keep the figure between the camera and the central star.
        d3d9::Matrix view;
        view._42 = 5;
        view._43 = 300 + distance;
        device.SetTransform(d3d9::D3DTS_VIEW, &view);

        // Projected size at the focal plane is proportional to focal length / distance.
        const float zoom = distance / shadowDollyDistance(20000);
        projection._11 *= zoom;
        projection._22 *= zoom;
    }
    d3d9::Matrix view;
    device.GetTransform(d3d9::D3DTS_VIEW,&view);
    // The figure is centred at world z=-300. Use projected proximity, so the
    // constant-size dolly zoom does not get mistaken for an actual close-up.
    const float depth=std::max(1.f,std::abs(view._43-300*view._33));
    const float proximity=std::clamp((projection._22/depth-0.0055f)/0.0035f,0.f,1.f);
    closeup=proximity*proximity*(3-2*proximity);
}
