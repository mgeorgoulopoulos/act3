#pragma once
class MeshGrid;
namespace extra {
inline bool creditsActive = false;
void layoutCredits(MeshGrid&);
void layoutCreditsBackground(MeshGrid&);
bool drawCreditsCard(unsigned long color, float sourceAspect = 4.f/3.f);
}
