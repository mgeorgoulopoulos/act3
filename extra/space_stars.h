#pragma once
class Scene3DS;
namespace d3d9 { class Device; }
namespace extra {
void releaseSpaceStars();
bool drawSpaceStars(d3d9::Device&, Scene3DS&);
}
