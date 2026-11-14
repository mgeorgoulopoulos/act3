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
#include <cstdlib>

namespace {
using Vertex = std::array<float, 3>;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
static_assert(sizeof(MODEL_VERTEX) == 8 * sizeof(float) &&
    offsetof(MODEL_VERTEX, nx) == 3 * sizeof(float) &&
    offsetof(MODEL_VERTEX, tu) == 6 * sizeof(float), "Unexpected MODEL_VERTEX layout");
struct PreviewMesh {
    Mesh3DS model;

    ~PreviewMesh()
    {
        if (model.pVB)
            model.pVB->Release();
        if (model.pIB)
            model.pIB->Release();
        std::free(model.VertexList);
        std::free(model.IndexList);
    }
};

PreviewMesh meshes[2];
Vertex center{};
float radius = 0;
}

bool preview::load()
{
    char path[] = ACT3_MESH_PATH;
    char names[][5] = {"Stem", "Egg"};
    d3d9::Device device;
    Vertex low, high;
    bool first = true;
    for (unsigned i = 0; i < 2; ++i) {
        auto& mesh = meshes[i].model;
        if (!mesh.LoadFrom3DS(path, names[i]) || !mesh.NumVertices ||
            !mesh.NumIndices || mesh.NumIndices % 3)
            return false;

        for (unsigned j = 0; j < mesh.NumVertices; ++j) {
            auto& v = mesh.VertexList[j];
            // The preview uses wireframe without lighting.
            v.nx = 0;
            v.ny = 1;
            v.nz = 0;
            const Vertex point{{v.x, v.y, v.z}};
            if (first) {
                low = high = point;
                first = false;
            }
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = (std::min)(low[axis], point[axis]);
                high[axis] = (std::max)(high[axis], point[axis]);
            }
        }

        mesh.CreateBuffers(&device);
        if (!mesh.pVB || !mesh.pIB || !mesh.RefreshBuffers())
            return false;
    }

    radius = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        center[axis] = (high[axis] + low[axis]) * 0.5f;
        const float extent = high[axis] - center[axis];
        radius += extent * extent;
    }
    radius = std::sqrt(radius);
    return radius > 0 && std::isfinite(radius);
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
    for (auto& mesh : meshes) {
        mesh.model.Transform = world;
        const int rendered = mesh.model.Render(&device);
        assert(rendered);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
