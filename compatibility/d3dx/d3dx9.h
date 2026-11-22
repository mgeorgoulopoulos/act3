#pragma once

#include <cstdint>
#include "../d3d9/indexed_draw.h"

using FLOAT = float;
#ifndef VOID
using VOID = void;
#endif
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
using D3DXVECTOR3 = d3d9::Vector3;
inline D3DXVECTOR3 operator-(const D3DXVECTOR3& a, const D3DXVECTOR3& b)
{ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline D3DXVECTOR3& operator+=(D3DXVECTOR3& a, const D3DXVECTOR3& b)
{ a.x+=b.x; a.y+=b.y; a.z+=b.z; return a; }
inline D3DXVECTOR3& operator/=(D3DXVECTOR3& a, float b)
{ a.x/=b; a.y/=b; a.z/=b; return a; }
inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* out, const D3DXVECTOR3* a, const D3DXVECTOR3* b)
{ const D3DXVECTOR3 result{a->y*b->z-a->z*b->y,a->z*b->x-a->x*b->z,a->x*b->y-a->y*b->x}; *out=result; return out; }
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
void D3DXMatrixRotationYawPitchRoll(D3DXMATRIXA16*, float, float, float);
inline void D3DXMatrixScaling(D3DXMATRIXA16* m, float x, float y, float z)
{ *m = {}; m->_11 = x; m->_22 = y; m->_33 = z; }
inline void D3DXMatrixTranslation(D3DXMATRIXA16* m, float x, float y, float z)
{ *m = {}; m->_41 = x; m->_42 = y; m->_43 = z; }
void D3DXMatrixPerspectiveFovLH(D3DXMATRIXA16*, float, float, float, float);
void D3DXVec3Normalize(D3DXVECTOR3*, const D3DXVECTOR3*);

inline void D3DXMatrixRotationX(D3DXMATRIXA16* m, float angle)
{ D3DXMatrixRotationYawPitchRoll(m, 0, angle, 0); }
inline void D3DXMatrixRotationY(D3DXMATRIXA16* m, float angle)
{ D3DXMatrixRotationYawPitchRoll(m, angle, 0, 0); }
void D3DXMatrixLookAtLH(D3DXMATRIXA16*, const D3DXVECTOR3* eye,
    const D3DXVECTOR3* target, const D3DXVECTOR3* up);

using PDIRECT3DTEXTURE9 = d3d9::BaseTexture*;
using D3DLOCKED_RECT = d3d9::LockedRect;
using d3d9::D3DPOOL_MANAGED;
constexpr unsigned D3DTOP_ADDSMOOTH = 11;
constexpr unsigned D3DBLEND_SRCALPHA = 5, D3DBLEND_INVSRCALPHA = 6;
