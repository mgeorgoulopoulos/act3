#pragma once
namespace d3d9 { class Device; struct BaseTexture; }
class MeshGrid;
namespace extra {
void prepareSpaceOverlays(d3d9::Device&, unsigned time);
bool drawSpaceFire(d3d9::Device&, d3d9::BaseTexture*, unsigned time);
bool drawSpaceDefinition(unsigned long color);
void layoutSpaceQuote(MeshGrid&);
}
