#include "indexed_draw.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <limits>

void d3d9::Device::SetStreamSource(const VertexBuffer& buffer, std::size_t vertexStride)
{
    vertexBuffer = &buffer;
    stride = vertexStride;
}

bool d3d9::Device::DrawIndexedPrimitive(PrimitiveType type, int baseVertex,
    unsigned minVertex, unsigned vertexCount, unsigned startIndex,
    unsigned primitiveCount) const
{
    const auto glLimit = (std::numeric_limits<GLsizei>::max)();
    const auto* vertices = vertexBuffer ? vertexBuffer->data() : nullptr;
    const auto* indices = indexBuffer ? indexBuffer->data() : nullptr;
    const auto indexCount = indexBuffer ? indexBuffer->size() : 0;
    if (type != D3DPT_TRIANGLELIST || fvf != D3DFVF_XYZ ||
        !vertices || !indices || baseVertex < 0 ||
        stride < 3 * sizeof(float) || stride % sizeof(float) != 0 ||
        stride > static_cast<std::size_t>(glLimit) ||
        startIndex > indexCount || primitiveCount > (indexCount - startIndex) / 3 ||
        primitiveCount > static_cast<unsigned>(glLimit / 3))
        return false;

    const auto streamCount = vertexBuffer->size() / (stride / sizeof(float));
    const auto base = static_cast<std::size_t>(baseVertex);
    if (base > streamCount || minVertex > streamCount - base ||
        vertexCount > streamCount - base - minVertex)
        return false;

    const auto count = static_cast<GLsizei>(primitiveCount * 3);
    const auto* firstIndex = indices + startIndex;
    for (GLsizei i = 0; i < count; ++i) {
        const auto index = firstIndex[i];
        if (index < minVertex || index - minVertex >= vertexCount)
            return false;
    }

    const auto* firstVertex = vertices + base * (stride / sizeof(float));
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, firstIndex);
    glDisableClientState(GL_VERTEX_ARRAY);
    return true;
}
