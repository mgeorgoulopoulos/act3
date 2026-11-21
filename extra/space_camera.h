#pragma once
namespace d3d9 { class Device; struct Matrix; }
namespace extra {
unsigned spaceCameraTime(unsigned sceneTime);
void applySpaceCamera(d3d9::Device&, unsigned time, d3d9::Matrix& projection);
}
