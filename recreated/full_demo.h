#pragma once
namespace d3d9 { class Device; }
namespace preview {
inline bool fullDemoActive=false;
bool loadFullDemo(d3d9::Device&);
void drawFullDemo(d3d9::Device&,int width,int height,unsigned time);
void releaseFullDemo();
constexpr unsigned fullDemoDuration=223200;
}
