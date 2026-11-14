#pragma once

#include <cstdint>
#include "../d3d9/indexed_draw.h"

using FLOAT = float;
using VOID = void;
using BOOL = int;
using INT = int;
#ifndef TRUE
constexpr BOOL TRUE = 1;
constexpr BOOL FALSE = 0;
#endif
#ifdef _WIN32
using DWORD = unsigned long;
#else
using DWORD = std::uint32_t;
#endif

using D3DXMATRIXA16 = d3d9::Matrix;
struct D3DXVECTOR2 { float x, y; };
struct D3DXVECTOR3 {
    float x, y, z;
    D3DXVECTOR3() = default;
    D3DXVECTOR3(float x, float y, float z) : x(x), y(y), z(z) {}
};
struct D3DXPLANE { float a, b, c, d; };
using D3DMATERIAL9 = d3d9::Material;
using D3DLIGHT9 = d3d9::Light;
using d3d9::D3DLIGHT_DIRECTIONAL;
using IDirect3DBaseTexture9 = d3d9::BaseTexture;
using LPDIRECT3DTEXTURE9 = d3d9::BaseTexture*;
using LPDIRECT3DCUBETEXTURE9 = d3d9::BaseTexture*;
d3d9::HRESULT D3DXCreateCubeTextureFromFile(d3d9::Device*, const char*,
    LPDIRECT3DCUBETEXTURE9*);
using d3d9::D3DFMT_DXT1;
using d3d9::D3DPOOL_DEFAULT;
constexpr unsigned D3DX_DEFAULT = ~0u;
#ifndef FAILED
inline bool FAILED(d3d9::HRESULT result) { return result < 0; }
#endif
d3d9::HRESULT D3DXCreateTextureFromFileEx(d3d9::Device*, const char*, unsigned,
    unsigned, unsigned, DWORD, d3d9::Format, d3d9::Pool, DWORD, DWORD, DWORD,
    void*, void*, LPDIRECT3DTEXTURE9*);
using IDirect3DVertexBuffer9 = d3d9::VertexBuffer;
using IDirect3DIndexBuffer9 = d3d9::IndexBuffer;
using PDIRECT3DDEVICE9 = d3d9::Device*;
using IDirect3DDevice9 = d3d9::Device;
using d3d9::D3DTS_VIEW;
using LPDIRECT3DDEVICE9 = d3d9::Device*;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
inline void D3DXMatrixIdentity(D3DXMATRIXA16* matrix) { *matrix = D3DXMATRIXA16{}; }
inline D3DXMATRIXA16* D3DXMatrixMultiply(D3DXMATRIXA16* out,
    const D3DXMATRIXA16* left, const D3DXMATRIXA16* right)
{
    *out = *left * *right;
    return out;
}
D3DXMATRIXA16* D3DXMatrixInverse(D3DXMATRIXA16*, float*, const D3DXMATRIXA16*);
D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXMATRIXA16*);
D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE*, const D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXVECTOR3*);
D3DXVECTOR3* D3DXPlaneIntersectLine(D3DXVECTOR3*, const D3DXPLANE*, const D3DXVECTOR3*, const D3DXVECTOR3*);
