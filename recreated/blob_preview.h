#pragma once
#include "compatibility/d3d9/indexed_draw.h"

namespace preview {
bool loadBlob(d3d9::Device& device);
void drawBlob(d3d9::Device& device, int width, int height, unsigned time);
void releaseBlob();
}
