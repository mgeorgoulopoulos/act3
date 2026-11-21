#pragma once
namespace d3d9 { class Device; }
class MeshGrid;
namespace extra {
void prepareSpaceOverlays(d3d9::Device&);
bool drawSpaceDefinition(unsigned long color);
void layoutSpaceQuote(MeshGrid&);
}
