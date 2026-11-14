#pragma once
#include "extra/mode.h"
#include "model.h"
#include "TextureManager.h"
#include <cstring>

class DemoEngine {
public:
    IDirect3DDevice9* m_pd3dDevice = nullptr;
    float m_fFPS = 0;
};

class Scene {
public:
    IDirect3DDevice9* m_pd3dDevice = nullptr;
    DemoEngine* m_pHostDemo = nullptr;
    DWORD m_dwTime = 0, m_dwDuration = 0;
    virtual ~Scene() = default;
    virtual bool Init() = 0;
    virtual void Render() = 0;
    virtual void Release() = 0;
};

using d3d9::Device;
using d3d9::Matrix;
using d3d9::D3DTS_WORLD;
using d3d9::D3DTS_VIEW;
using d3d9::D3DTS_PROJECTION;
using d3d9::D3DPT_TRIANGLELIST;
using d3d9::D3DRS_LIGHTING;
using d3d9::D3DRS_AMBIENT;
using d3d9::D3DRS_SPECULARENABLE;
using d3d9::D3DRS_LOCALVIEWER;
using d3d9::D3DRS_ZENABLE;
using d3d9::D3DRS_ZWRITEENABLE;
using d3d9::D3DRS_ALPHABLENDENABLE;
using d3d9::D3DRS_CULLMODE;
using d3d9::D3DRS_SRCBLEND;
using d3d9::D3DRS_DESTBLEND;
using d3d9::D3DBLEND_ONE;
using d3d9::D3DCULL_NONE;
using d3d9::D3DCULL_CW;
using d3d9::D3DCULL_CCW;
using d3d9::D3DTSS_TEXCOORDINDEX;
using d3d9::D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;
using d3d9::D3DTSS_TCI_PASSTHRU;
constexpr float D3DX_PI = 3.14159265358979323846f;
constexpr unsigned D3DCLEAR_TARGET = 1, D3DCLEAR_ZBUFFER = 2;
constexpr unsigned D3DSAMP_MAGFILTER = 5, D3DSAMP_MINFILTER = 6, D3DSAMP_MIPFILTER = 7;
constexpr unsigned D3DTEXF_LINEAR = 2;
inline unsigned D3DCOLOR_ARGB(unsigned a, unsigned r, unsigned g, unsigned b)
{ return (a << 24) | (r << 16) | (g << 8) | b; }
inline void ZeroMemory(void* dst, std::size_t size) { std::memset(dst, 0, size); }

void PutText(IDirect3DDevice9*, int, int, const char*);
unsigned long GetColorAtPosition(IDirect3DDevice9*, float, float, float);
unsigned long FlareIntensity(IDirect3DDevice9*, float, float, float);
void GetXY(IDirect3DDevice9*, float, float, float, int*, int*);
void GetScreenCenter(int*, int*);
void RenderQuadAt(IDirect3DDevice9*, int, int, float, unsigned long);
void setSceneViewport(int, int);
