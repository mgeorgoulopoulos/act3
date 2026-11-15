#pragma once

class MeshGrid;
namespace d3d9 { class Device; }

namespace extra {
void positionGreet(d3d9::Device& device, MeshGrid& grid, float x, float y, float z);
}
