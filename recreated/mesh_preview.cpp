#include "mesh_preview.h"
#include "eggplant_cubemap.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cassert>
#include "../model.h"
#include "../TextureManager.h"

extern TextureManager TexMan;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetObject
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
Scene3DS scene{};
Mesh3DS* meshes[2]{};
Material* materials[2]{};
Vertex center{};
const auto placements = preview::eggplantPlacements();
float radius = 0;
LPDIRECT3DCUBETEXTURE9 eggplantCube = nullptr;
}

bool preview::load()
{
    char path[] = ACT3_MESH_PATH;
    char names[][5] = {"Stem", "Egg"};
    d3d9::Device device;
    if (!scene.LoadFromS3D(path, &device))
        return false;
    Vertex low, high;
    bool first = true;
    for (unsigned i = 0; i < 2; ++i) {
        auto* mesh = scene.GetObject(names[i]);
        if (!mesh || !mesh->NumVertices || !mesh->pVB || !mesh->pIB)
            return false;
        meshes[i] = mesh;
        materials[i] = scene.MaMa.GetMaterial(mesh->MaterialName);
        if (!materials[i])
            return false;
        for (unsigned j = 0; j < mesh->NumVertices; ++j) {
            const auto& v = mesh->VertexList[j];
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
    }

    const auto meshLow = low;
    const auto meshHigh = high;
    for (int axis = 0; axis < 3; ++axis) {
        low[axis] += placements.front().m[3][axis];
        high[axis] += placements.front().m[3][axis];
    }
    for (const auto& placement : placements)
        for (int axis = 0; axis < 3; ++axis) {
            low[axis] = (std::min)(low[axis], meshLow[axis] + placement.m[3][axis]);
            high[axis] = (std::max)(high[axis], meshHigh[axis] + placement.m[3][axis]);
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
    if (!eggplantCube) {
        const auto result = loadEggplantCubemap(&device, eggplantCube);
        assert(result == d3d9::D3D_OK);
    }
    if (eggplantCube)
        device.SetTextureStageState(0, d3d9::D3DTSS_TEXCOORDINDEX,
            d3d9::D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1);
    for (unsigned i = 0; i < 2; ++i) {
        materials[i]->Set(&device, eggplantCube);
        for (const auto& placement : placements) {
            meshes[i]->Transform = placement * world;
            const int rendered = meshes[i]->Render(&device);
            assert(rendered);
        }
    }
    device.SetTextureStageState(0, d3d9::D3DTSS_TEXCOORDINDEX,
        d3d9::D3DTSS_TCI_PASSTHRU);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    char loading[] = "loading.png";
    TexMan.SetTexture(&device, 0, loading);
    if (!glIsEnabled(GL_TEXTURE_2D))
        return;
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(12, 12);
    glTexCoord2f(1, 0); glVertex2i(172, 12);
    glTexCoord2f(1, 1); glVertex2i(172, 92);
    glTexCoord2f(0, 1); glVertex2i(12, 92);
    glEnd();
    TexMan.SetNoTexture(&device);
}

void preview::releaseTextures()
{
    if (eggplantCube) {
        eggplantCube->Release();
        eggplantCube = nullptr;
    }
    for (unsigned i = 0; i < scene.NumObjects; ++i) {
        auto& mesh = scene.ObjectList[i];
        if (mesh.pVB) mesh.pVB->Release();
        if (mesh.pIB) mesh.pIB->Release();
        std::free(mesh.VertexList);
        std::free(mesh.IndexList);
    }
    delete[] scene.ObjectList;
    std::free(scene.MaMa.MaterialList);
    TexMan.Release();
    TexMan.pFirstEntry = nullptr;
}
