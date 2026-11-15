#include "mesh_preview.h"
#include "shadow_preview.h"
#include "shadow_blur.h"
#include "../extra/mode.h"
#include "../extra/eggplant_grade.h"
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
#include <algorithm>

extern TextureManager TexMan;
namespace {
d3d9::Device device;
DemoEngine host;
Scene1 scene{};
std::chrono::steady_clock::time_point playbackStart;
constexpr unsigned shadowDuration = 56000;
constexpr unsigned eggplantDuration = 40000;
// Reference captures show a sharp silhouette during ordinary playback.
constexpr float shadowBlurOffset = 0;
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
    const bool wantsShadow = shadowSelected || sequenceSelected;
    if (wantsShadow && !loadShadow(device)) {
        fs::current_path(previous, error);
        return false;
    }
    if (shadowSelected) {
        fs::current_path(previous, error);
        playbackStart = std::chrono::steady_clock::now();
        return !error;
    }
    host.m_pd3dDevice = &device;
    scene.m_pHostDemo = &host;
    scene.m_dwDuration = eggplantDuration;
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
    extra::setEggplantViewport(width, height);
    glEnable(GL_NORMALIZE);
    glColor3f(1, 1, 1);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - playbackStart).count();
    const auto cycle = static_cast<unsigned>(elapsed %
        (sequenceSelected ? shadowDuration + eggplantDuration :
            shadowSelected ? shadowDuration : eggplantDuration));
    if (shadowSelected || (sequenceSelected && cycle < shadowDuration)) {
        const int source = (std::min)({512, width, height});
        setSceneViewport(source, source);
        glViewport(0, 0, source, source);
        drawShadow(device, cycle);
        recreated::presentShadowBlur(source, width, height, shadowBlurOffset);
        return;
    }
    scene.m_dwTime = sequenceSelected ? cycle - shadowDuration : cycle;
    scene.Render();
    extra::postProcessEggplants(width, height);
}

void preview::releaseTextures()
{
    extra::releaseEggplantGrade();
    recreated::releaseShadowBlur();
    if (shadowSelected || sequenceSelected)
        releaseShadow();
    if (scene.m_pd3dDevice)
        scene.Release();
    if (scene.CubeMap)
        scene.CubeMap->Release();
    std::free(scene.scene3d.MaMa.MaterialList);
    std::free(scene.floor1.MaMa.MaterialList);
    TexMan.Release();
    TexMan.pFirstEntry = nullptr;
}
