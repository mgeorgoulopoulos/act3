#pragma once
namespace d3d9 { class Device; }

namespace extra {
inline bool enabled = false;
bool applyEggplantCamera(d3d9::Device&, unsigned milliseconds);
float eggplantFovScale(unsigned milliseconds);
}
