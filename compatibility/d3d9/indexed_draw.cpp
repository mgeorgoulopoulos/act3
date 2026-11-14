#include "indexed_draw.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <limits>

#include <new>

namespace d3d9 {
namespace {
constexpr unsigned modelFormat = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
}

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
        sharedHandle || (format != D3DFVF_XYZ && format != modelFormat))
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

HRESULT Device::SetMaterial(const Material* material)
{
    if (!material)
        return D3DERR_INVALIDCALL;
    const auto apply = [](GLenum property, const Color& color) {
        const GLfloat rgba[] = {color.r, color.g, color.b, color.a};
        glMaterialfv(GL_FRONT_AND_BACK, property, rgba);
    };
    apply(GL_AMBIENT, material->Ambient);
    apply(GL_DIFFUSE, material->Diffuse);
    apply(GL_SPECULAR, material->Specular);
    apply(GL_EMISSION, material->Emissive);
    const float power = material->Power;
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, power > 0 ? (power < 128 ? power : 128) : 0);
    return D3D_OK;
}

HRESULT Device::SetTexture(unsigned stage, BaseTexture* texture)
{
    if (stage != 0)
        return D3DERR_INVALIDCALL;
    constexpr GLenum cubeMap = 0x8513;
    glDisable(GL_TEXTURE_2D);
    glDisable(cubeMap);
    if (texture) {
        glBindTexture(texture->target, texture->name);
        glEnable(texture->target);
    }
    return D3D_OK;
}

HRESULT Device::SetTransform(TransformState state, const Matrix* matrix)
{
    if (!matrix)
        return D3DERR_INVALIDCALL;
    switch (state) {
    case D3DTS_WORLD: world = *matrix; break;
    case D3DTS_VIEW: view = *matrix; break;
    case D3DTS_PROJECTION: projection = *matrix; break;
    default: return D3DERR_INVALIDCALL;
    }
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
    const bool model = fvf == modelFormat;
    const auto minimumStride = (model ? 8 : 3) * sizeof(float);
    if (type != D3DPT_TRIANGLELIST || (!model && fvf != D3DFVF_XYZ) ||
        !vertices || !indices || baseVertex < 0 ||
        stride < minimumStride || stride % sizeof(float) != 0 ||
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
    // Convert D3D clip depth [0,w] to OpenGL clip depth [-w,w].
    Matrix clip;
    clip.m[2][2] = 2;
    clip.m[3][2] = -1;
    const auto glProjection = projection * clip;
    const auto modelView = world * view;
    // OpenGL reads the same memory as column-major, transposing both matrices.
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&glProjection.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&modelView.m[0][0]);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex);
    if (model) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, static_cast<GLsizei>(stride), firstVertex + 3);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex + 6);
    }
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, firstIndex);
    if (model) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    return D3D_OK;
}
}
