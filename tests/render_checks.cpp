#include "compatibility/d3dx/d3dx9.h"
#include "model.h"
#include "extra/space_stars.h"
#include "extra/mode.h"
#include <GL/glx.h>
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

// Geometry fixtures do not invoke the historical mesh renderer.
Mesh3DS::Mesh3DS() {}
Scene3DS::Scene3DS() {}

namespace {
bool checkLetter(d3d9::Device& device, const char* name, bool invalidEnum)
{
    while (glGetError() != GL_NO_ERROR) {}
    // Loading an asset must not attribute an earlier draw error to its upload.
    if (invalidEnum) glEnable(0xffffffffu);
    else glEnd();
    LPDIRECT3DTEXTURE9 texture{};
    const auto result = D3DXCreateTextureFromFileEx(&device, name, 0, 0,
        D3DX_DEFAULT, 0, d3d9::D3DFMT_DXT1, d3d9::D3DPOOL_DEFAULT,
        D3DX_DEFAULT, D3DX_DEFAULT, 0, nullptr, nullptr, &texture);
    if (result < 0 || !texture) {
        std::fprintf(stderr, "FAIL %s: preceding GL error discarded a valid texture\n", name);
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, texture->name);
    std::vector<unsigned char> pixels(128 * 128 * 4);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    unsigned lo = 255, hi = 0;
    for (unsigned i = 0; i < pixels.size(); i += 4) {
        lo = std::min(lo, unsigned(pixels[i]));
        hi = std::max(hi, unsigned(pixels[i]));
    }
    const bool ready = glGetError() == GL_NO_ERROR && lo < 20 && hi > 230;
    texture->Release();
    std::printf("%s %s: black background and visible glyph\n", ready ? "PASS" : "FAIL", name);
    return ready;
}
std::array<float, 3> direction(unsigned face, float u, float v)
{
    switch (face) {
    case 0: return {1, -v, -u}; case 1: return {-1, -v, u};
    case 2: return {u, 1, v}; case 3: return {u, -1, -v};
    case 4: return {u, -v, 1}; default: return {-u, -v, -1};
    }
}
std::array<float, 2> coordinates(unsigned face, const std::array<float, 3>& d)
{
    const auto x = d[0], y = d[1], z = d[2];
    switch (face) {
    case 0: return {-z, -y}; case 1: return {z, -y};
    case 2: return {x, z}; case 3: return {x, -z};
    case 4: return {x, -y}; default: return {-x, -y};
    }
}
bool checkStars(d3d9::Device& device)
{
    MODEL_VERTEX vertices[2]{};
    vertices[0].x=vertices[0].y=vertices[0].z=-10;
    vertices[1].x=vertices[1].y=vertices[1].z=10;
    Mesh3DS meshes[2];
    for(auto& mesh:meshes){mesh.VertexList=vertices;mesh.NumVertices=2;}
    Scene3DS scene;scene.NumObjects=2;scene.ObjectList=meshes;
    d3d9::Matrix view,projection;
    D3DXMatrixPerspectiveFovLH(&projection,1.f,1.f,0.05f,10000);
    device.SetTransform(d3d9::D3DTS_VIEW,&view);
    device.SetTransform(d3d9::D3DTS_PROJECTION,&projection);
    glViewport(0,0,64,64);glClearColor(0,0,0,1);
    glDepthMask(GL_TRUE);glClearDepth(0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    using Active=void (*)(GLenum);
    auto active=reinterpret_cast<Active>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glActiveTexture")));
    GLuint black;glGenTextures(1,&black);active(0x84c1);
    glBindTexture(GL_TEXTURE_2D,black);const unsigned char pixel[]={0,0,0,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glEnable(GL_TEXTURE_2D);active(0x84c0);
    extra::enabled=false;
    bool ready=!extra::drawSpaceStars(device,scene);
    extra::enabled=true;
    ready=extra::drawSpaceStars(device,scene)&&ready;
    std::array<unsigned char,64*64*4> pixels{};
    glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    unsigned visible=0;
    for(unsigned i=0;i<pixels.size();i+=4)if(pixels[i]+pixels[i+1]+pixels[i+2]>30)++visible;
    ready=ready&&visible>50&&glIsEnabled(GL_DEPTH_TEST);
    active(0x84c1);ready=ready&&glIsEnabled(GL_TEXTURE_2D);glDisable(GL_TEXTURE_2D);active(0x84c0);
    ready=ready&&glGetError()==GL_NO_ERROR;
    glDeleteTextures(1,&black);extra::releaseSpaceStars();extra::enabled=false;
    glDisable(GL_DEPTH_TEST);glClearDepth(1);
    std::printf("%s anniversary starfield: %u visible pixels despite inherited depth/texture state; Party bypass preserved\n",ready?"PASS":"FAIL",visible);
    return ready;
}
bool checkChrome(d3d9::Device& device)
{
    while (glGetError() != GL_NO_ERROR) {}
    LPDIRECT3DCUBETEXTURE9 texture{};
    if (D3DXCreateCubeTextureFromFile(&device, "data/textures/chrome.dds", &texture) < 0 || !texture)
        return false;
    constexpr unsigned size = 256, cube = 0x8513, positiveX = 0x8515;
    std::array<std::vector<unsigned char>, 6> faces;
    glBindTexture(cube, texture->name);
    for (unsigned face = 0; face < 6; ++face) {
        faces[face].resize(size * size * 4);
        glGetTexImage(positiveX + face, 0, GL_RGBA, GL_UNSIGNED_BYTE, faces[face].data());
    }
    const auto pixel = [&](unsigned face, float u, float v) {
        const auto x = std::clamp(int((u + 1) * size / 2), 0, int(size - 1));
        const auto y = std::clamp(int((v + 1) * size / 2), 0, int(size - 1));
        return int(faces[face][(y * size + x) * 4]);
    };
    double total = 0, worst = 0;
    for (unsigned face = 0; face < 6; ++face) for (unsigned edge = 0; edge < 4; ++edge) {
        double error = 0;
        for (unsigned k = 0; k < size; ++k) {
            const float along = 2 * (k + 0.5f) / size - 1;
            const float u = edge == 0 ? -1 : edge == 1 ? 1 : along;
            const float v = edge == 2 ? -1 : edge == 3 ? 1 : along;
            const auto d = direction(face, u, v);
            unsigned neighbor = 0;
            for (unsigned axis = 0; axis < 3; ++axis)
                if (axis != face / 2 && std::abs(d[axis]) == 1)
                    neighbor = axis * 2 + (d[axis] < 0);
            const auto uv = coordinates(neighbor, d);
            error += std::abs(pixel(face, u, v) - pixel(neighbor, uv[0], uv[1]));
        }
        error /= size; total += error; worst = std::max(worst, error);
    }
    const bool ready = glGetError() == GL_NO_ERROR && worst < 10;
    texture->Release();
    std::printf("%s chrome cubemap edge continuity: mean %.2f, worst %.2f / 255\n",
                ready ? "PASS" : "FAIL", total / 24, worst);
    return ready;
}
}
int main()
{
    Display* display = XOpenDisplay(nullptr);
    if (!display) return 2;
    int attributes[] = {GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, None};
    auto* visual = glXChooseVisual(display, DefaultScreen(display), attributes);
    if (!visual) return 2;
    XSetWindowAttributes settings{};
    settings.colormap = XCreateColormap(display, DefaultRootWindow(display), visual->visual, AllocNone);
    auto window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 64, 64, 0,
        visual->depth, InputOutput, visual->visual, CWColormap, &settings);
    auto context = glXCreateContext(display, visual, nullptr, True);
    XFree(visual);
    XMapWindow(display,window);XSync(display,False);
    if (!context || !glXMakeCurrent(display, window, context)) return 2;
    bool ready;
    {
        d3d9::Device device;
        ready = checkLetter(device, "data/textures/act3/R.png", true);
        ready = checkLetter(device, "data/textures/act3/E.png", false) && ready;
        ready = checkChrome(device) && ready;
        ready = checkStars(device) && ready;
    }
    glXMakeCurrent(display, None, nullptr);
    glXDestroyContext(display, context);
    XDestroyWindow(display, window);
    XFreeColormap(display, settings.colormap);
    XCloseDisplay(display);
    return ready ? 0 : 1;
}
