#pragma once

namespace d3d9 { class Device; }

namespace preview {
bool loadShadow(d3d9::Device& device);
void drawShadow(d3d9::Device& device, unsigned elapsed);
void releaseShadow();
}
