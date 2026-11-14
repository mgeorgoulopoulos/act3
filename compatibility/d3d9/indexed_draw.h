#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace d3d9 {
// Borrowed CPU buffers. The current backend supports XYZ and 16-bit indices.
constexpr unsigned D3DFVF_XYZ = 0x002;
enum PrimitiveType { D3DPT_TRIANGLELIST = 4 };

template<class Element>
class Buffer {
public:
    explicit Buffer(std::size_t count) : storage(count) {}

    Element* Lock()
    {
        if (locked)
            return nullptr;
        locked = true;
        return storage.data();
    }

    void Unlock() { locked = false; }
    const Element* data() const { return locked ? nullptr : storage.data(); }
    std::size_t size() const { return storage.size(); }

private:
    std::vector<Element> storage;
    bool locked = false;
};

using VertexBuffer = Buffer<float>;
using IndexBuffer = Buffer<std::uint16_t>;

class Device {
public:
    void SetStreamSource(const VertexBuffer& buffer, std::size_t stride);
    void SetIndices(const IndexBuffer& buffer) { indexBuffer = &buffer; }
    void SetFVF(unsigned format) { fvf = format; }
    bool DrawIndexedPrimitive(PrimitiveType type, int baseVertex, unsigned minVertex,
        unsigned vertexCount, unsigned startIndex, unsigned primitiveCount) const;

private:
    const VertexBuffer* vertexBuffer = nullptr;
    const IndexBuffer* indexBuffer = nullptr;
    std::size_t stride = 0;
    unsigned fvf = 0;
};
}
