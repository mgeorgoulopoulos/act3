#include "blob_preview.h"
#include "greetz_scene.h"
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

extern TextureManager TexMan;

namespace {
Scene2 scene{};
DemoEngine host{};
std::chrono::steady_clock::time_point previousFrame;
GLuint previousImage;
int imageWidth, imageHeight;

void restorePreviousImage(int width, int height)
{
    if (!previousImage || imageWidth != width || imageHeight != height)
        return;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, previousImage);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    // Retain the original's faint presentation trails without relying on swap behavior.
    glColor3f(0.72f, 0.72f, 0.72f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glPopAttrib();
}

void savePreviousImage(int width, int height)
{
    if (!previousImage)
        glGenTextures(1, &previousImage);
    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D, previousImage);
    if (imageWidth != width || imageHeight != height) {
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 0, 0, width, height, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        imageWidth = width;
        imageHeight = height;
    } else {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    }
    glPopAttrib();
}
}

bool preview::loadBlob(d3d9::Device& device)
{
    host.m_pd3dDevice = &device;
    scene.m_pHostDemo = &host;
    scene.m_dwDuration = 19300;
    previousFrame = {};
    return scene.Init();
}

void preview::drawBlob(d3d9::Device& device, int width, int height, unsigned time)
{
    const auto now = std::chrono::steady_clock::now();
    const float delta = std::chrono::duration<float>(now - previousFrame).count();
    host.m_fFPS = previousFrame.time_since_epoch().count() && delta > 0 ? 1.f / delta : 60.f;
    previousFrame = now;
    scene.m_dwTime = time;
    device.SetProjectionHorizontalScale(4.f * height / (3.f * width));
    device.Clear(0, nullptr, D3DCLEAR_TARGET, 0xff000000, 1, 0);
    restorePreviousImage(width, height);
    scene.Render();
    savePreviousImage(width, height);
    device.SetProjectionHorizontalScale(1);
}

void preview::releaseBlob()
{
    glDeleteTextures(1, &previousImage);
    previousImage = 0;
    imageWidth = imageHeight = 0;
    scene.Release();
    scene.m3.Release();
    scene = {};
    TexMan.Release();
}
