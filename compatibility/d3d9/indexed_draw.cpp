#include "indexed_draw.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <limits>

#include <new>

namespace d3d9 {
Device::~Device()
{
    SetStreamSource(0, nullptr, 0, 0);
    SetIndices(nullptr);
}

HRESULT Device::CreateVertexBuffer(unsigned length, unsigned usage, unsigned format,
    Pool pool, VertexBuffer** buffer, void** sharedHandle)
{
    if (!buffer)
        return D3DERR_INVALIDCALL;
    *buffer = nullptr;
    if (!length || (usage & ~D3DUSAGE_WRITEONLY) || pool != D3DPOOL_SYSTEMMEM ||
        sharedHandle || format != D3DFVF_XYZ)
        return D3DERR_INVALIDCALL;
    try {
        *buffer = new VertexBuffer(length);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY_RESULT;
    }
    return D3D_OK;
}

HRESULT Device::CreateIndexBuffer(unsigned length, unsigned usage, Format format,
    Pool pool, IndexBuffer** buffer, void** sharedHandle)
{
    if (!buffer)
        return D3DERR_INVALIDCALL;
    *buffer = nullptr;
    if (!length || length % sizeof(std::uint16_t) || (usage & ~D3DUSAGE_WRITEONLY) ||
        format != D3DFMT_INDEX16 || pool != D3DPOOL_SYSTEMMEM || sharedHandle)
        return D3DERR_INVALIDCALL;
    try {
        *buffer = new IndexBuffer(length);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY_RESULT;
    }
    return D3D_OK;
}

HRESULT Device::SetStreamSource(unsigned stream, VertexBuffer* buffer, unsigned offset,
    unsigned vertexStride)
{
    if (stream || (buffer && (offset >= buffer->length || offset % sizeof(float) ||
                             !vertexStride || vertexStride % sizeof(float))))
        return D3DERR_INVALIDCALL;
    if (buffer)
        buffer->AddRef();
    if (vertexBuffer)
        vertexBuffer->Release();
    vertexBuffer = buffer;
    streamOffset = offset;
    stride = vertexStride;
    return D3D_OK;
}

HRESULT Device::SetIndices(IndexBuffer* buffer)
{
    if (buffer)
        buffer->AddRef();
    if (indexBuffer)
        indexBuffer->Release();
    indexBuffer = buffer;
    return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitive(PrimitiveType type, int baseVertex,
    unsigned minVertex, unsigned vertexCount, unsigned startIndex,
    unsigned primitiveCount) const
{
    const auto glLimit = (std::numeric_limits<GLsizei>::max)();
    const auto* vertices = vertexBuffer && !vertexBuffer->locked ? vertexBuffer->storage.data() : nullptr;
    const auto* indices = indexBuffer && !indexBuffer->locked ? indexBuffer->storage.data() : nullptr;
    const auto indexCount = indexBuffer ? indexBuffer->storage.size() : 0;
    if (type != D3DPT_TRIANGLELIST || fvf != D3DFVF_XYZ ||
        !vertices || !indices || baseVertex < 0 ||
        stride < 3 * sizeof(float) || stride % sizeof(float) != 0 ||
        stride > static_cast<std::size_t>(glLimit) ||
        startIndex > indexCount || primitiveCount > (indexCount - startIndex) / 3 ||
        primitiveCount > static_cast<unsigned>(glLimit / 3))
        return D3DERR_INVALIDCALL;

    const auto streamCount = (vertexBuffer->length - streamOffset) / stride;
    const auto base = static_cast<std::size_t>(baseVertex);
    if (base > streamCount || minVertex > streamCount - base ||
        vertexCount > streamCount - base - minVertex)
        return D3DERR_INVALIDCALL;

    const auto count = static_cast<GLsizei>(primitiveCount * 3);
    const auto* firstIndex = indices + startIndex;
    for (GLsizei i = 0; i < count; ++i) {
        const auto index = firstIndex[i];
        if (index < minVertex || index - minVertex >= vertexCount)
            return D3DERR_INVALIDCALL;
    }

    const auto* firstVertex = vertices + (streamOffset + base * stride) / sizeof(float);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, firstIndex);
    glDisableClientState(GL_VERTEX_ARRAY);
    return D3D_OK;
}
}
