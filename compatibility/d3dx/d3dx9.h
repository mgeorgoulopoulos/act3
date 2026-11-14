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
using D3DMATERIAL9 = d3d9::Material;
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
using LPDIRECT3DDEVICE9 = d3d9::Device*;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
inline void D3DXMatrixIdentity(D3DXMATRIXA16* matrix) { *matrix = D3DXMATRIXA16{}; }
