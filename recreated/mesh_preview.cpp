#include "mesh_preview.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cassert>
#include "../fvf.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

namespace {
using Vertex = std::array<float, 3>;
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};
struct ReleaseBuffer {
    template<class Buffer>
    void operator()(Buffer* buffer) const { buffer->Release(); }
};

struct DrawMesh {
    std::unique_ptr<d3d9::VertexBuffer, ReleaseBuffer> vertices;
    std::unique_ptr<d3d9::IndexBuffer, ReleaseBuffer> indices;
    unsigned vertexCount = 0;
    unsigned primitiveCount = 0;

    bool upload(d3d9::Device& device, const Mesh& mesh)
    {
        using namespace d3d9;
        vertexCount = static_cast<unsigned>(mesh.vertices.size());
        primitiveCount = static_cast<unsigned>(mesh.indices.size() / 3);
        VertexBuffer* vb = nullptr;
        if (device.CreateVertexBuffer(vertexCount * 3 * sizeof(float),
                D3DUSAGE_WRITEONLY, D3DFVF_XYZ, D3DPOOL_SYSTEMMEM, &vb, nullptr) < 0)
            return false;
        vertices.reset(vb);
        IndexBuffer* ib = nullptr;
        if (device.CreateIndexBuffer(static_cast<unsigned>(mesh.indices.size() * sizeof(std::uint16_t)),
                D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_SYSTEMMEM, &ib, nullptr) < 0)
            return false;
        indices.reset(ib);

        void* data = nullptr;
        if (vb->Lock(0, 0, &data, 0) < 0)
            return false;
        auto* destination = static_cast<float*>(data);
        for (const auto& vertex : mesh.vertices)
            destination = std::copy(vertex.begin(), vertex.end(), destination);
        if (vb->Unlock() < 0 || ib->Lock(0, 0, &data, 0) < 0)
            return false;
        std::copy(mesh.indices.begin(), mesh.indices.end(), static_cast<std::uint16_t*>(data));
        return ib->Unlock() == D3D_OK;
    }
};
std::vector<Mesh> meshes;
std::vector<DrawMesh> drawMeshes;
Vertex center{};
float radius = 0;

bool readMesh(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    unsigned short count;
    if (!file.read(reinterpret_cast<char*>(&count), sizeof(count)) || !count)
        return false;
    std::vector<COMMON_VERTEX> vertices(count);
    if (!file.read(reinterpret_cast<char*>(vertices.data()),
                   vertices.size() * sizeof(COMMON_VERTEX)) ||
        !file.read(reinterpret_cast<char*>(&count), sizeof(count)) || !count || count % 3)
        return false;
    Mesh mesh;
    mesh.indices.resize(count);
    if (!file.read(reinterpret_cast<char*>(mesh.indices.data()),
                   mesh.indices.size() * sizeof(unsigned short)))
        return false;
    for (auto index : mesh.indices)
        if (index >= vertices.size())
            return false;
    for (const auto& v : vertices)
        mesh.vertices.push_back({{v.x, v.y, v.z}});
    meshes.push_back(std::move(mesh));
    return true;
}
}

bool preview::load()
{
    meshes.clear();
    drawMeshes.clear();
    if (!readMesh(ACT3_STEM_PATH) || !readMesh(ACT3_EGG_PATH))
        return false;
    Vertex low = meshes.front().vertices.front();
    Vertex high = low;
    for (const auto& mesh : meshes)
        for (const auto& vertex : mesh.vertices)
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = (std::min)(low[axis], vertex[axis]);
                high[axis] = (std::max)(high[axis], vertex[axis]);
            }
    radius = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        center[axis] = (high[axis] + low[axis]) * 0.5f;
        const float extent = high[axis] - center[axis];
        radius += extent * extent;
    }
    radius = std::sqrt(radius);
    if (!(radius > 0 && std::isfinite(radius)))
        return false;
    d3d9::Device device;
    std::vector<DrawMesh> uploaded;
    for (const auto& mesh : meshes) {
        uploaded.emplace_back();
        if (!uploaded.back().upload(device, mesh))
            return false;
    }
    drawMeshes.swap(uploaded);
    meshes.clear();
    return true;
}

void preview::draw(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    const double aspect = double(width) / height;
    const double x = aspect > 1 ? aspect : 1;
    const double y = aspect < 1 ? 1 / aspect : 1;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-x, x, -y, y, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(-65, 1, 0, 0);
    glRotatef(25, 0, 0, 1);
    const float scale = 0.86f / radius;
    glScalef(scale, scale, scale);
    glTranslatef(-center[0], -center[1], -center[2]);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.79f, 0.57f, 1.0f);
    d3d9::Device device;
    device.SetFVF(d3d9::D3DFVF_XYZ);
    for (const auto& mesh : drawMeshes) {
        device.SetStreamSource(0, mesh.vertices.get(), 0, 3 * sizeof(float));
        device.SetIndices(mesh.indices.get());
        const auto result = device.DrawIndexedPrimitive(d3d9::D3DPT_TRIANGLELIST,
            0, 0, mesh.vertexCount, 0, mesh.primitiveCount);
        assert(result == d3d9::D3D_OK);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
