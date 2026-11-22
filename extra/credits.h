#pragma once
class MeshGrid;
namespace d3d9 { class Device; struct Matrix; }
namespace extra {
inline bool creditsActive = false;
void layoutCredits(MeshGrid&);
void layoutCreditsBackground(MeshGrid&);
void zoomCreditsWorld(d3d9::Matrix&);
void restoreCreditsLens(d3d9::Device&);
bool drawCreditsCard(unsigned long color, float sourceAspect = 4.f/3.f);
}
