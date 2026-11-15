#pragma once
namespace d3d9 { class Device; struct Matrix; }

namespace extra {
inline bool enabled = false;
void setEggplantViewport(int width, int height);
void applyEggplantCamera(d3d9::Device&, unsigned milliseconds, d3d9::Matrix& projection);
}
