#include "greetz_layout.h"
#include "mode.h"
#include "model.h"

void extra::positionGreet(d3d9::Device& device, MeshGrid& grid, float x, float y, float z)
{
    if (enabled) {
        d3d9::Matrix projection;
        device.GetTransform(d3d9::D3DTS_PROJECTION, &projection);
        const float aspectRelativeToOriginal = 0.75f * projection._22 / projection._11;
        // Use half the extra width for placement and half as margin for the original sprites.
        x *= (1.f + aspectRelativeToOriginal) * 0.5f;
    }
    grid.SetTransform(TRANSFORM_TRANSLATE, x, y, z);
}
