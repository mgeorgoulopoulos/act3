#pragma once
#include "compatibility/d3dx/d3dx9.h"
#include <vector>

namespace preview {
std::vector<d3d9::Matrix> eggplantPlacements();
d3d9::HRESULT loadEggplantCubemap(d3d9::Device* device, LPDIRECT3DCUBETEXTURE9& cube);
}
