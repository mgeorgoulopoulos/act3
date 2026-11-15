#include "shadow_preview.h"
#include "shadow_scene.h"
#include <cstdlib>

namespace {
Scene4 playback{};
DemoEngine host{};
}

void Scene4::Render()
{
    RenderEverything();
}

void Scene4::Release()
{
    Billboard.Release();
    Star.Release();
    shadow.Release();
    room.Release();
    std::free(shadow.MaMa.MaterialList);
    std::free(room.MaMa.MaterialList);
}

bool preview::loadShadow(d3d9::Device& device)
{
    host.m_pd3dDevice = &device;
    playback.m_pHostDemo = &host;
    playback.m_dwDuration = 56000;
    return playback.Init();
}

void preview::drawShadow(d3d9::Device&, unsigned elapsed)
{
    playback.m_dwTime = elapsed;
    playback.Render();
}

void preview::releaseShadow()
{
    playback.Release();
}
