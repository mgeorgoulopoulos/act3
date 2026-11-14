#include "mesh_preview.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cassert>
#include "../model.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>

namespace {
using Vertex = std::array<float, 3>;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
static_assert(sizeof(MODEL_VERTEX) == 8 * sizeof(float) &&
    offsetof(MODEL_VERTEX, nx) == 3 * sizeof(float) &&
    offsetof(MODEL_VERTEX, tu) == 6 * sizeof(float), "Unexpected MODEL_VERTEX layout");
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
        if (device.CreateVertexBuffer(static_cast<unsigned>(vertexCount * sizeof(MODEL_VERTEX)),
                D3DUSAGE_WRITEONLY, FVF_MODEL, D3DPOOL_SYSTEMMEM, &vb, nullptr) < 0)
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
        auto* destination = static_cast<unsigned char*>(data);
        for (const auto& vertex : mesh.vertices) {
            const MODEL_VERTEX model{vertex[0], vertex[1], vertex[2], 0, 0, 1, 0, 0};
            std::memcpy(destination, &model, sizeof(model));
            destination += sizeof(model);
        }
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
    const float scale = 0.86f / radius;
    d3d9::Matrix world;
    for (int axis = 0; axis < 3; ++axis) {
        world.m[axis][axis] = scale;
        world.m[3][axis] = -center[axis] * scale;
    }
    constexpr float radians = 3.14159265358979323846f / 180;
    const float pitch = -65 * radians;
    const float yaw = 25 * radians;
    d3d9::Matrix pitchRotation, yawRotation;
    pitchRotation.m[1][1] = pitchRotation.m[2][2] = std::cos(pitch);
    pitchRotation.m[1][2] = std::sin(pitch);
    pitchRotation.m[2][1] = -pitchRotation.m[1][2];
    yawRotation.m[0][0] = yawRotation.m[1][1] = std::cos(yaw);
    yawRotation.m[0][1] = std::sin(yaw);
    yawRotation.m[1][0] = -yawRotation.m[0][1];
    const auto view = yawRotation * pitchRotation;
    d3d9::Matrix projection;
    projection.m[0][0] = static_cast<float>(1 / x);
    projection.m[1][1] = static_cast<float>(1 / y);
    projection.m[2][2] = -0.25f;
    projection.m[3][2] = 0.5f;
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.79f, 0.57f, 1.0f);
    d3d9::Device device;
    device.SetTransform(d3d9::D3DTS_VIEW, &view);
    device.SetTransform(d3d9::D3DTS_PROJECTION, &projection);
    for (const auto& mesh : drawMeshes) {
        Mesh3DS historical;
        historical.Transform = world;
        historical.pVB = mesh.vertices.get();
        historical.pIB = mesh.indices.get();
        historical.NumVertices = static_cast<unsigned short>(mesh.vertexCount);
        historical.NumIndices = static_cast<unsigned short>(mesh.primitiveCount * 3);
        const int rendered = historical.Render(&device);
        assert(rendered);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
