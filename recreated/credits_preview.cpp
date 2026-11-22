#include "credits_preview.h"
#include "credits_scene.h"
#include "extra/credits.h"
#include "extra/color_grade.h"
#include <algorithm>

extern TextureManager TexMan;
extern bool Saving;
namespace { Scene3 scene{}; DemoEngine host{}; }

bool preview::loadCredits(d3d9::Device& device)
{
    host.m_pd3dDevice = &device;
    scene.m_pHostDemo = &host;
    scene.m_dwDuration = 38000;
    Saving = false;
    extra::creditsActive = true;
    return scene.Init() && scene.CubeMap && scene.Cylinder.pVB && scene.Particles.pVB;
}

void preview::drawCredits(d3d9::Device& device, int width, int height, unsigned time)
{
    scene.m_dwTime = std::min(time,37999u);
    host.m_fFPS = 60;
    device.SetProjectionHorizontalScale(extra::enabled ? 4.f*height/(3.f*width) : 1.f);
    scene.Render();
    device.SetProjectionHorizontalScale(1);
    extra::applyColorGrade(width,height,extra::SceneGrade::Credits);
}

void preview::releaseCredits()
{
    scene.Cylinder.Release();
    scene.Cylinder2.Release();
    scene.Particles.Release();
    scene.m2.Release();
    scene.Names.Release();
    if (scene.CubeMap) scene.CubeMap->Release();
    scene = {};
    TexMan.Release();
    extra::creditsActive = false;
}
