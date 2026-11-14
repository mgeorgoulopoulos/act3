#include "mesh_preview.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#ifdef GetObject
#undef GetObject
#endif
#ifdef ZeroMemory
#undef ZeroMemory
#endif
#include "../EggplantScene.h"
#include <chrono>
#include <filesystem>
#include <cstdlib>

extern TextureManager TexMan;
namespace {
d3d9::Device device;
DemoEngine host;
Scene1 scene{};
std::chrono::steady_clock::time_point playbackStart;
}

bool preview::load()
{
    namespace fs = std::filesystem;
    std::error_code error;
    const auto previous = fs::current_path(error);
    if (error)
        return false;
    fs::current_path(ACT3_ASSET_ROOT, error);
    if (error)
        return false;
    host.m_pd3dDevice = &device;
    scene.m_pHostDemo = &host;
    scene.m_dwDuration = 40000;
    const bool loaded = scene.Init();
    fs::current_path(previous, error);
    playbackStart = std::chrono::steady_clock::now();
    return loaded && !error && scene.CubeMap && scene.floor1.ObjectList &&
        scene.Seq1.m_dwNumShots >= 2 && scene.light_source.pVB;
}

void preview::draw(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    setSceneViewport(width, height);
    glViewport(0, 0, width, height);
    glEnable(GL_NORMALIZE);
    glColor3f(1, 1, 1);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - playbackStart).count();
    scene.m_dwTime = static_cast<DWORD>(elapsed % scene.m_dwDuration);
    scene.Render();
}

void preview::releaseTextures()
{
    if (scene.m_pd3dDevice)
        scene.Release();
    if (scene.CubeMap)
        scene.CubeMap->Release();
    std::free(scene.scene3d.MaMa.MaterialList);
    std::free(scene.floor1.MaMa.MaterialList);
    TexMan.Release();
    TexMan.pFirstEntry = nullptr;
}
