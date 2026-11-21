#include "space_preview.h"
#include "space_scene.h"
#include "extra/space_engraving.h"
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

extern TextureManager TexMan;
namespace {
SpaceScene scene{};
DemoEngine host{};
class TransitionEnd : public Scene {
public:
    bool Init() override { return true; }
    void Render() override { m_pd3dDevice->Clear(0, nullptr, 3, 0xff000000, 1, 0); }
    void Release() override {}
} ending;
}
bool preview::loadSpace(d3d9::Device& device)
{
    host.m_pd3dDevice = &device;
    scene.m_pHostDemo = &host;
    scene.m_dwDuration = 70000;
    ending.m_pd3dDevice = &device;
    scene.m_pNextScene = &ending;
    return scene.Init();
}
void preview::drawSpace(unsigned time)
{
    scene.m_dwTime = time;
    if (extra::enabled) {
        GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
        scene.m_pd3dDevice->SetProjectionHorizontalScale(4.f*viewport[3]/(3.f*viewport[2]));
    }
    scene.Render();
    scene.m_pd3dDevice->SetProjectionHorizontalScale(1);
}
void preview::releaseSpace()
{
    extra::releaseSpaceEngraving();
    scene.Man.Release();
    scene.Woman.Release();
    scene.Release();
    for (auto* model : {&scene.Pioneer, &scene.Sympan, &scene.Man, &scene.Woman})
        std::free(model->MaMa.MaterialList);
    if(scene.CubeMap) scene.CubeMap->Release();
    if(scene.p_empty_tex) scene.p_empty_tex->Release();
    if(scene.tex_transition) scene.tex_transition->Release();
    scene = {};
    TexMan.Release();
}
