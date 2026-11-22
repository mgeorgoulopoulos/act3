#pragma once

namespace d3d9 { class Device; struct Matrix; }

namespace extra {
float shadowDollyDistance(unsigned milliseconds);
float shadowCloseupStrength();
void applyShadowCamera(d3d9::Device&, unsigned milliseconds, d3d9::Matrix& projection);
}
