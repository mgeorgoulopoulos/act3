#pragma once
namespace d3d9 { class Device; }
namespace preview {
bool loadSpace(d3d9::Device&);
void drawSpace(unsigned);
void releaseSpace();
}
