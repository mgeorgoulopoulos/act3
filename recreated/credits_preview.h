#pragma once
namespace d3d9 { class Device; }
namespace preview {
bool loadCredits(d3d9::Device&);
void drawCredits(d3d9::Device&, int width, int height, unsigned time);
void releaseCredits();
}
