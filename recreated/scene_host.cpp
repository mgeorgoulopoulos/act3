#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
#ifdef GetObject
#undef GetObject
#endif
#ifdef ZeroMemory
#undef ZeroMemory
#endif
#include "scene_host.h"
#include <algorithm>
#include <cmath>

char text_buffer[1024];
DWORD frames = 0;
void PutText(IDirect3DDevice9*, int, int, const char*) {}
int screenWidth, screenHeight;
static bool project(Device* device, float x, float y, float z, int* sx, int* sy, float* depth)
{
    Matrix w, v, p;
    device->GetTransform(D3DTS_WORLD, &w);
    device->GetTransform(D3DTS_VIEW, &v);
    device->GetTransform(D3DTS_PROJECTION, &p);
    const auto m = w * v * p;
    float clip[4];
    for (int i = 0; i < 4; ++i)
        clip[i] = x*m.m[0][i] + y*m.m[1][i] + z*m.m[2][i] + m.m[3][i];
    if (clip[3] <= 0 || std::abs(clip[0]) >= clip[3] || std::abs(clip[1]) >= clip[3] ||
        clip[2] < 0 || clip[2] > clip[3])
        return false;
    *sx = int((1 + clip[0]/clip[3]) * screenWidth / 2);
    *sy = int((1 - clip[1]/clip[3]) * screenHeight / 2);
    *depth = clip[2]/clip[3];
    return true;
}

void GetXY(Device* device, float x, float y, float z, int* sx, int* sy)
{
    float depth;
    *sx = *sy = 0;
    project(device, x, y, z, sx, sy, &depth);
}

void GetScreenCenter(int* x, int* y) { *x = screenWidth/2; *y = screenHeight/2; }

unsigned long GetColorAtPosition(Device* device, float x, float y, float z)
{
    int sx, sy; float depth;
    if (!project(device, x, y, z, &sx, &sy, &depth))
        return 0;
    unsigned char pixel[4]{};
    glReadPixels(sx, screenHeight-1-sy, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return (0xfful << 24) | (static_cast<unsigned long>(pixel[0]) << 16) |
           (static_cast<unsigned long>(pixel[1]) << 8) | pixel[2];
}

unsigned long FlareIntensity(Device* device, float x, float y, float z)
{
    int sx, sy; float depth;
    if (!project(device, x, y, z, &sx, &sy, &depth))
        return 0;
    const float intensity = (std::min)({1.0f, 10.0f*sx/screenWidth,
        10.0f*(screenWidth-sx)/screenWidth, 10.0f*sy/screenHeight,
        10.0f*(screenHeight-sy)/screenHeight});
    return static_cast<unsigned long>(255*intensity) * 0x01010101u;
}

static void drawScreenQuad(int x, int y, float half, float depth, unsigned long color)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor4ub((color >> 16) & 255, (color >> 8) & 255, color & 255, (color >> 24) & 255);
    const float z = 1 - 2*depth;
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(x-half,y-half,z);
    glTexCoord2f(1,0); glVertex3f(x+half,y-half,z);
    glTexCoord2f(1,1); glVertex3f(x+half,y+half,z);
    glTexCoord2f(0,1); glVertex3f(x-half,y+half,z);
    glEnd();
}

void RenderQuadAt(Device*, int x, int y, float size, unsigned long color)
{
    drawScreenQuad(x, y, size*screenWidth/2, 0, color);
}

void setSceneViewport(int width, int height) { screenWidth = width; screenHeight = height; }
