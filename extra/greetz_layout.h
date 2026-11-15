#pragma once

class MeshGrid;
namespace d3d9 { class Device; }

namespace extra {
bool drawGreetz(d3d9::Device& device, MeshGrid& grid, unsigned time);
}
