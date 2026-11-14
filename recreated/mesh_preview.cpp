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
    Material material{};

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

bool buildNormals(Mesh3DS& mesh)
{
    for (unsigned i = 0; i < mesh.NumIndices; i += 3) {
        const auto* face = mesh.IndexList + i;
        if (face[0] >= mesh.NumVertices || face[1] >= mesh.NumVertices ||
            face[2] >= mesh.NumVertices)
            return false;
        auto& a = mesh.VertexList[face[0]];
        auto& b = mesh.VertexList[face[1]];
        auto& c = mesh.VertexList[face[2]];
        const Vertex u{{b.x-a.x, b.y-a.y, b.z-a.z}};
        const Vertex v{{c.x-a.x, c.y-a.y, c.z-a.z}};
        for (auto* vertex : {&a, &b, &c}) {
            vertex->nx += u[1]*v[2] - u[2]*v[1];
            vertex->ny += u[2]*v[0] - u[0]*v[2];
            vertex->nz += u[0]*v[1] - u[1]*v[0];
        }
    }
    return true;
}


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
            !mesh.NumIndices || mesh.NumIndices % 3 ||
            !meshes[i].material.LoadFrom3DS(path, mesh.MaterialName))
            return false;

        for (unsigned j = 0; j < mesh.NumVertices; ++j) {
            auto& v = mesh.VertexList[j];
            v.nx = v.ny = v.nz = 0;
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

        if (!buildNormals(mesh))
            return false;
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
    // The historical loader swaps Y and Z. Keep the preview's original view.
    d3d9::Matrix previewAxes;
    previewAxes.m[1][1] = previewAxes.m[2][2] = 0;
    previewAxes.m[1][2] = previewAxes.m[2][1] = 1;
    world = world * previewAxes;
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
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const GLfloat light[] = {-0.4f, 0.7f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    d3d9::Device device;
    device.SetTransform(d3d9::D3DTS_VIEW, &view);
    device.SetTransform(d3d9::D3DTS_PROJECTION, &projection);
    for (auto& mesh : meshes) {
        mesh.material.Set(&device, nullptr);
        mesh.model.Transform = world;
        const int rendered = mesh.model.Render(&device);
        assert(rendered);
    }
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}
