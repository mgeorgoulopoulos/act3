#include "mesh_preview.h"
#include "eggplant_cubemap.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cassert>
#include "../model.h"
#include "../camera.h"
#include <chrono>
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
using namespace d3d9;
using Vertex = std::array<float, 3>;
using d3d9::D3DFVF_XYZ;
using d3d9::D3DFVF_NORMAL;
using d3d9::D3DFVF_TEX1;
static_assert(sizeof(MODEL_VERTEX) == 8 * sizeof(float) &&
    offsetof(MODEL_VERTEX, nx) == 3 * sizeof(float) &&
    offsetof(MODEL_VERTEX, tu) == 6 * sizeof(float), "Unexpected MODEL_VERTEX layout");
Scene3DS scene{};
Scene3DS room{};
Mesh3DS* meshes[2]{};
::Material* materials[2]{};
const auto placements = preview::eggplantPlacements();
CameraSequence camera;
static_assert(sizeof(CameraShot) == 28, "Unexpected camera record layout");
std::chrono::steady_clock::time_point playbackStart;
LPDIRECT3DCUBETEXTURE9 eggplantCube = nullptr;

void drawRoom(d3d9::Device& device)
{
    // Load after the GL context is current so the S3D loader can upload circuit.png.
    if (!room.ObjectList) {
        char path[] = ACT3_ROOM_PATH;
        if (!room.LoadFromS3D(path, &device)) {
            WriteToLog("Could not load c_room.s3d.");
            return;
        }
    }

    d3d9::Matrix transform;
    transform._11 = transform._22 = transform._33 = 400;
    transform._42 = 50;
    device.SetRenderState(D3DRS_LIGHTING, 0);
    device.SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    device.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    glColor3f(1, 1, 1);
    // BatchRender composes the scene transform into each object in place.
    room.ClearTransform();
    room.Transform = transform;
    const auto began = device.BeginScene();
    assert(began == D3D_OK);
    room.BatchRender(&device);
    const auto ended = device.EndScene();
    assert(ended == D3D_OK);
    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device.SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
}

void drawEggplants(d3d9::Device& device, bool reflected, CULLINFO& frustum)
{
    constexpr auto coord = d3d9::D3DTSS_TEXCOORDINDEX;
    if (eggplantCube)
        device.SetTextureStageState(0, coord, d3d9::D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1);
    device.SetRenderState(D3DRS_CULLMODE, reflected ? D3DCULL_CW : D3DCULL_CCW);
    d3d9::Matrix reflection;
    reflection._22 = -1;
    for (unsigned i = 0; i < 2; ++i) {
        materials[i]->Set(&device, eggplantCube);
        for (const auto& placement : placements) {
            meshes[i]->Transform = reflected ? reflection * placement : placement;
            meshes[i]->TransformBoundingBox();
            meshes[i]->CalculateBBPlanes();
            if (!ObjectInFrustum(&frustum, meshes[i]->BoundingBoxScratch, meshes[i]->BBPlanes))
                continue;
            const int rendered = meshes[i]->Render(&device);
            assert(rendered);
        }
    }
    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device.SetTextureStageState(0, coord, d3d9::D3DTSS_TCI_PASSTHRU);
}

void releaseScene(Scene3DS& loaded)
{
    for (unsigned i = 0; i < loaded.NumObjects; ++i) {
        auto& mesh = loaded.ObjectList[i];
        if (mesh.pVB) mesh.pVB->Release();
        if (mesh.pIB) mesh.pIB->Release();
        std::free(mesh.VertexList);
        std::free(mesh.IndexList);
    }
    delete[] loaded.ObjectList;
    std::free(loaded.MaMa.MaterialList);
    loaded.ObjectList = nullptr;
    loaded.NumObjects = 0;
    loaded.MaMa.MaterialList = nullptr;
}
}

bool preview::load()
{
    char path[] = ACT3_MESH_PATH;
    char names[][5] = {"Stem", "Egg"};
    d3d9::Device device;
    if (!scene.LoadFromS3D(path, &device))
        return false;
    for (unsigned i = 0; i < 2; ++i) {
        meshes[i] = scene.GetObject(names[i]);
        if (!meshes[i] || !meshes[i]->pVB || !meshes[i]->pIB)
            return false;
        materials[i] = scene.MaMa.GetMaterial(meshes[i]->MaterialName);
        if (!materials[i])
            return false;
    }
    char cameraPath[] = ACT3_CAMERA_PATH;
    if (!camera.Load(cameraPath) || camera.m_dwNumShots < 2)
        return false;
    playbackStart = std::chrono::steady_clock::now();
    return camera.m_dwTotalTime > camera.m_pKeyframes[0].TimeDelta;
}

void preview::draw(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    d3d9::Device device;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    d3d9::Matrix projection;
    constexpr float nearPlane = 1.0f, farPlane = 10000.0f;
    const float yScale = 1.0f / std::tan(3.14159265358979323846f / 8);
    projection._11 = yScale / (4.0f / 3.0f);
    projection._22 = yScale;
    projection._33 = farPlane / (farPlane - nearPlane);
    projection._34 = 1;
    projection._43 = -nearPlane * projection._33;
    projection._44 = 0;
    device.SetRenderState(D3DRS_ZENABLE, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const GLfloat light[] = {-0.4f, 0.7f, 1.0f, 0.0f};
    const GLfloat diffuse[] = {0.9f, 0.9f, 0.9f, 1};
    const GLfloat specular[] = {0.1f, 0.1f, 0.1f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    device.SetRenderState(D3DRS_AMBIENT, 0xffffffff);
    device.SetRenderState(D3DRS_LOCALVIEWER, 1);
    device.SetRenderState(D3DRS_SPECULARENABLE, 1);
    glEnable(GL_LIGHT0);
    device.SetRenderState(D3DRS_LIGHTING, 1);
    glEnable(GL_NORMALIZE);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - playbackStart).count();
    const auto duration = camera.m_dwTotalTime - camera.m_pKeyframes[0].TimeDelta;
    camera.Cerp(static_cast<unsigned long>(elapsed % duration));
    camera.UpdateViewMatrix(&device);
    device.SetTransform(d3d9::D3DTS_PROJECTION, &projection);
    d3d9::Matrix view;
    device.GetTransform(d3d9::D3DTS_VIEW, &view);
    CULLINFO frustum;
    UpdateCullInfo(&frustum, &view, &projection);
    if (!eggplantCube) {
        const auto result = loadEggplantCubemap(&device, eggplantCube);
        assert(result == d3d9::D3D_OK);
    }
    drawEggplants(device, true, frustum);
    // Scene1 disables lighting for the floor and leaves it off for the main grid.
    drawRoom(device);
    drawEggplants(device, false, frustum);
    glDisable(GL_NORMALIZE);
    device.SetRenderState(D3DRS_LIGHTING, 0);
    glDisable(GL_LIGHT0);

    char loading[] = "loading.png";
    TexMan.SetTexture(&device, 0, loading);
    if (!glIsEnabled(GL_TEXTURE_2D))
        return;
    device.SetRenderState(D3DRS_ZENABLE, 0);
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
    releaseScene(room);
    releaseScene(scene);
    TexMan.Release();
    TexMan.pFirstEntry = nullptr;
}
