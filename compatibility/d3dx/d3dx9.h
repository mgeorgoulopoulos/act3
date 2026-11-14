#pragma once

#include <cstdint>
#include "../d3d9/indexed_draw.h"

using FLOAT = float;
using VOID = void;
#ifdef _WIN32
using DWORD = unsigned long;
#else
using DWORD = std::uint32_t;
#endif

using D3DXMATRIXA16 = d3d9::Matrix;
struct D3DXVECTOR3 { float x, y, z; };
struct D3DXPLANE { float a, b, c, d; };
struct D3DCOLORVALUE { float r, g, b, a; };
struct D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive;
    float Power;
};
struct IDirect3DBaseTexture9;
using IDirect3DVertexBuffer9 = d3d9::VertexBuffer;
using IDirect3DIndexBuffer9 = d3d9::IndexBuffer;
using PDIRECT3DDEVICE9 = d3d9::Device*;
using LPDIRECT3DDEVICE9 = d3d9::Device*;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
inline void D3DXMatrixIdentity(D3DXMATRIXA16* matrix) { *matrix = D3DXMATRIXA16{}; }
