#include "mesh_preview.h"
#include "blob_preview.h"
#include "space_preview.h"
#include "credits_preview.h"
#include "full_demo.h"
#include "music.h"
#include "shadow_preview.h"
#include "shadow_blur.h"
#include "../extra/shadow_focus.h"
#include "../extra/mode.h"
#include "../extra/color_grade.h"
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
bool spaceSelected = false;
bool creditsSelected = false;
bool blobSelected = false;
bool fullSelected = true, originalRequested = false, demoFinished = false;
constexpr unsigned blobDuration = 19300;
constexpr unsigned shadowDuration = 56000;
constexpr unsigned eggplantDuration = 40000;
// Reference captures show a sharp silhouette during ordinary playback.
constexpr float shadowBlurOffset = 0;
}

void preview::selectOption(std::string_view argument)
{
    if (argument == "--extra") {
        originalRequested = false;
        extra::enabled = true;
    }
    else if (argument == "--original") {
        originalRequested = true;
        extra::enabled = false;
    } else if (argument == "--demo") {
        fullSelected = true;
        if (!originalRequested) extra::enabled = true;
        spaceSelected = creditsSelected = blobSelected = shadowSelected = sequenceSelected = false;
    }
    else if (argument == "--credits") {
        fullSelected = false;
        creditsSelected = true;
        spaceSelected = blobSelected = shadowSelected = sequenceSelected = false;
    }
    else if (argument == "--space") {
        fullSelected = false;
        spaceSelected = true;
        blobSelected = shadowSelected = sequenceSelected = false;
    } else if (argument == "--blob") {
        fullSelected = false;
        blobSelected = true;
        shadowSelected = sequenceSelected = false;
    } else if (argument == "--shadow") {
        fullSelected = false;
        blobSelected = false;
        shadowSelected = true;
        sequenceSelected = false;
    } else if (argument == "--sequence") {
        fullSelected = false;
        blobSelected = false;
        sequenceSelected = true;
        shadowSelected = false;
    }
}

const char* preview::windowTitle()
{
    if (fullSelected) return "ACT3 | Full Demo + Music";
    if (creditsSelected) return "ACT3 | Credits";
    if (spaceSelected) return "ACT3 | Space";
    if (blobSelected)
        return "ACT3 | Blob";
    if (sequenceSelected)
        return extra::enabled ? "ACT3 | Shadow to Eggplant Extra" : "ACT3 | Shadow to Eggplant";
    if (shadowSelected)
        return extra::enabled ? "ACT3 | Shadow Extra" : "ACT3 | Shadow";
    return extra::enabled ? "ACT3 | Extra" : "ACT3 | OpenGL";
}

bool preview::load()
{
    if (fullSelected && !originalRequested) extra::enabled = true;
    namespace fs = std::filesystem;
    std::error_code error;
    const auto previous = fs::current_path(error);
    if (error)
        return false;
    fs::current_path(ACT3_ASSET_ROOT, error);
    if (error)
        return false;
    if (fullSelected) {
        const bool loaded = loadFullDemo(device) && music::load("data/Universus.mp3");
        fs::current_path(previous,error);
        return loaded && !error;
    }
    if (creditsSelected) {
        const bool loaded = loadCredits(device);
        fs::current_path(previous,error);
        playbackStart = std::chrono::steady_clock::now();
        return loaded && !error;
    }
    if (spaceSelected) {
        const bool loaded = loadSpace(device);
        fs::current_path(previous, error);
        playbackStart = std::chrono::steady_clock::now();
        return loaded && !error;
    }
    if (blobSelected) {
        const bool loaded = loadBlob(device);
        fs::current_path(previous, error);
        playbackStart = std::chrono::steady_clock::now();
        return loaded && !error;
    }
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
    if (fullSelected) {
        if (!music::start()) {
            demoFinished = true;
            return;
        }
        const auto time = music::milliseconds();
        music::features();
        drawFullDemo(device,width,height,static_cast<unsigned>(time));
        demoFinished = time >= fullDemoDuration;
        return;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - playbackStart).count();
    if (creditsSelected) {
        drawCredits(device,width,height,static_cast<unsigned>(std::min<decltype(elapsed)>(elapsed,37999)));
        return;
    }
    if (spaceSelected) {
        drawSpace(static_cast<unsigned>(elapsed % 70000));
        return;
    }
    if (blobSelected) {
        drawBlob(device, width, height, static_cast<unsigned>(elapsed % blobDuration));
        return;
    }
    const auto cycle = static_cast<unsigned>(elapsed %
        (sequenceSelected ? shadowDuration + eggplantDuration :
            shadowSelected ? shadowDuration : eggplantDuration));
    if (shadowSelected || (sequenceSelected && cycle < shadowDuration)) {
        if (extra::enabled) {
            device.SetProjectionHorizontalScale((4.f * height) / (3.f * width));
            drawShadow(device, cycle);
            device.SetProjectionHorizontalScale(1.f);
            extra::applyColorGrade(width, height, extra::SceneGrade::Shadow, cycle);
            extra::postProcessShadow(width, height, cycle);
            return;
        }
        const int source = (std::min)({512, width, height});
        setSceneViewport(source, source);
        glViewport(0, 0, source, source);
        drawShadow(device, cycle);
        recreated::presentShadowBlur(source, width, height, shadowBlurOffset);
        return;
    }
    scene.m_dwTime = sequenceSelected ? cycle - shadowDuration : cycle;
    scene.Render();
    extra::applyColorGrade(width, height, extra::SceneGrade::Eggplants);
}

void preview::releaseTextures()
{
    releasePresentation();
    if (fullSelected) {
        music::release();
        releaseFullDemo();
        return;
    }
    if (creditsSelected) {
        extra::releaseColorGrade();
        releaseCredits();
        return;
    }
    if (spaceSelected) {
        releaseSpace();
        return;
    }
    if (blobSelected) {
        releaseBlob();
        return;
    }
    extra::releaseColorGrade();
    recreated::releaseShadowBlur();
    extra::releaseShadowFocus();
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

bool preview::finished() { return demoFinished; }
