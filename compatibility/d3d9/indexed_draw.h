#pragma once

#include <cstddef>
#include <cstdint>

namespace d3d9 {
// Borrowed CPU buffers. The current backend supports XYZ and 16-bit indices.
constexpr unsigned D3DFVF_XYZ = 0x002;
enum PrimitiveType { D3DPT_TRIANGLELIST = 4 };

class Device {
public:
    void SetStreamSource(const void* vertices, std::size_t stride, unsigned count);
    void SetIndices(const std::uint16_t* indices, std::size_t count);
    void SetFVF(unsigned format) { fvf = format; }
    bool DrawIndexedPrimitive(PrimitiveType type, int baseVertex, unsigned minVertex,
        unsigned vertexCount, unsigned startIndex, unsigned primitiveCount) const;

private:
    const void* vertices = nullptr;
    const std::uint16_t* indices = nullptr;
    std::size_t stride = 0;
    unsigned streamCount = 0;
    std::size_t indexCount = 0;
    unsigned fvf = 0;
};
}
