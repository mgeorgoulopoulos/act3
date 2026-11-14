#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../d3dx/matrix.h"

namespace d3d9 {
struct Color { float r, g, b, a; };
enum LightType { D3DLIGHT_DIRECTIONAL = 3 };
struct Light {
    LightType Type;
    Color Diffuse, Specular, Ambient;
    struct { float x, y, z; } Direction;
    float Range;
};
struct Material {
    Color Diffuse, Ambient, Specular, Emissive;
    float Power;
};
struct BaseTexture {
    unsigned name;
    unsigned target;
    void Release();
};
using HRESULT = std::int32_t;
constexpr HRESULT D3D_OK = 0;
constexpr HRESULT D3DERR_INVALIDCALL = -2005530516;
constexpr HRESULT E_OUTOFMEMORY_RESULT = -2147024882;
constexpr unsigned D3DFVF_XYZ = 0x002;
constexpr unsigned D3DFVF_NORMAL = 0x010;
constexpr unsigned D3DFVF_TEX1 = 0x100;
constexpr unsigned D3DUSAGE_WRITEONLY = 0x008;
enum PrimitiveType { D3DPT_TRIANGLELIST = 4 };
enum Pool { D3DPOOL_DEFAULT = 0, D3DPOOL_SYSTEMMEM = 2 };
enum Format { D3DFMT_INDEX16 = 101, D3DFMT_DXT1 = 0x31545844 };
enum TransformState { D3DTS_VIEW = 2, D3DTS_PROJECTION = 3, D3DTS_WORLD = 256 };
enum TextureStageState { D3DTSS_TEXCOORDINDEX = 11 };
enum RenderState {
    D3DRS_ZENABLE = 7, D3DRS_ZWRITEENABLE = 14, D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20, D3DRS_CULLMODE = 22, D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_SPECULARENABLE = 29, D3DRS_LIGHTING = 137, D3DRS_AMBIENT = 139,
    D3DRS_LOCALVIEWER = 142
};
enum CullMode { D3DCULL_NONE = 1, D3DCULL_CW = 2, D3DCULL_CCW = 3 };
constexpr unsigned D3DBLEND_ONE = 2;
constexpr unsigned D3DTSS_TCI_PASSTHRU = 0;
constexpr unsigned D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR = 0x00030000;

template<class Element>
class Buffer {
public:
    HRESULT Lock(unsigned offset, unsigned size, void** data, unsigned flags)
    {
        if (!data)
            return D3DERR_INVALIDCALL;
        *data = nullptr;
        if (locked || flags || offset >= length || size > length - offset)
            return D3DERR_INVALIDCALL;
        locked = true;
        *data = reinterpret_cast<unsigned char*>(storage.data()) + offset;
        return D3D_OK;
    }

    HRESULT Unlock()
    {
        if (!locked)
            return D3DERR_INVALIDCALL;
        locked = false;
        return D3D_OK;
    }

    unsigned AddRef() { return ++references; }
    unsigned Release()
    {
        const auto remaining = --references;
        if (!remaining)
            delete this;
        return remaining;
    }

private:
    friend class Device;
    explicit Buffer(unsigned bytes)
        : storage(bytes / sizeof(Element) + (bytes % sizeof(Element) != 0)),
          length(bytes) {}
    ~Buffer() = default;

    std::vector<Element> storage;
    unsigned length;
    unsigned references = 1;
    bool locked = false;
};

using VertexBuffer = Buffer<float>;
using IndexBuffer = Buffer<std::uint16_t>;

class Device {
public:
    Device() = default;
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    HRESULT CreateVertexBuffer(unsigned length, unsigned usage, unsigned format,
        Pool pool, VertexBuffer** buffer, void** sharedHandle);
    HRESULT CreateIndexBuffer(unsigned length, unsigned usage, Format format,
        Pool pool, IndexBuffer** buffer, void** sharedHandle);
    HRESULT SetStreamSource(unsigned stream, VertexBuffer* buffer, unsigned offset,
        unsigned vertexStride);
    HRESULT SetIndices(IndexBuffer* buffer);
    HRESULT SetFVF(unsigned format) { fvf = format; return D3D_OK; }
    HRESULT SetTransform(TransformState state, const Matrix* matrix);
    HRESULT GetTransform(TransformState state, Matrix* matrix) const;
    HRESULT SetLight(unsigned index, const Light* light);
    HRESULT LightEnable(unsigned index, int enable);
    HRESULT SetMaterial(const Material* material);
    HRESULT SetTexture(unsigned stage, BaseTexture* texture);
    HRESULT SetTextureStageState(unsigned stage, TextureStageState state, unsigned value);
    HRESULT SetRenderState(RenderState state, unsigned value);
    HRESULT BeginScene()
    {
        if (inScene)
            return D3DERR_INVALIDCALL;
        inScene = true;
        return D3D_OK;
    }
    HRESULT EndScene()
    {
        if (!inScene)
            return D3DERR_INVALIDCALL;
        inScene = false;
        return D3D_OK;
    }
    HRESULT DrawIndexedPrimitive(PrimitiveType type, int baseVertex, unsigned minVertex,
        unsigned vertexCount, unsigned startIndex, unsigned primitiveCount) const;

private:
    VertexBuffer* vertexBuffer = nullptr;
    IndexBuffer* indexBuffer = nullptr;
    unsigned streamOffset = 0;
    unsigned stride = 0;
    unsigned fvf = 0;
    Matrix world, view, projection;
    Light light0{};
    bool light0Enabled = false;
    bool inScene = false;
    unsigned sourceBlend = D3DBLEND_ONE;
    unsigned destinationBlend = 1; // D3DBLEND_ZERO
};
}
