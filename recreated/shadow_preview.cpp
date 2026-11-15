#include "shadow_preview.h"
#include "scene_host.h"
#include "camera.h"
#include <cstdlib>

namespace {
Scene3DS shadow{}, room{};
CameraSequence camera;
MeshBillboard particles{}, star{};
Mesh3DS* roomMesh;
Material* roomMaterial;

float randomBetween(float from, float to)
{
    return from + (to - from) * (std::rand() % 1000 / 1000.f);
}

bool loadParticles(d3d9::Device& device)
{
    if (!particles.Init(&device, 250) || !particles.pVB ||
        !star.Init(&device, 1) || !star.pVB)
        return false;
    for (unsigned i = 0; i < particles.NumParticles; ++i) {
        auto& particle = particles.ParticleList[i];
        const auto red = static_cast<unsigned>(randomBetween(30, 200));
        randomBetween(0, 0); // Preserve the original random sequence.
        const auto blue = static_cast<unsigned>(randomBetween(30, 150));
        const float radius = randomBetween(20, 50);
        const float roll = randomBetween(0, 2 * D3DX_PI);
        const float pitch = randomBetween(0, 2 * D3DX_PI);
        D3DXVECTOR3 position{radius, 0, 0};
        D3DXMATRIXA16 rotation;
        D3DXMatrixRotationYawPitchRoll(&rotation, 0, pitch, roll);
        D3DXVec3TransformCoord(&position, &position, &rotation);
        particle.x = position.x;
        particle.y = position.y;
        particle.z = position.z;
        particle.size = randomBetween(20, 60);
        particle.color = (red << 16) | blue;
        particle.alive = true;
    }
    star.ParticleList[0].size = 1000;
    star.ParticleList[0].color = 0xffffffff;
    star.ParticleList[0].alive = true;
    return true;
}
}

extern TextureManager TexMan;

bool preview::loadShadow(d3d9::Device& device)
{
    if (!shadow.LoadFromS3D("data/geometry/shadow.s3d", &device) ||
        !room.LoadFromS3D("data/geometry/room.s3d", &device) ||
        !camera.Load("data/geometry/shadow.cam") || camera.m_dwNumShots < 2 ||
        camera.m_dwTotalTime <= camera.m_pKeyframes[0].TimeDelta)
        return false;
    char roomName[] = "Room_12_Sh";
    roomMesh = room.GetObject(roomName);
    roomMaterial = roomMesh ? room.MaMa.GetMaterial(roomMesh->MaterialName) : nullptr;
    if (!roomMaterial || !loadParticles(device))
        return false;
    TexMan.LoadTexture(&device, "star.png");
    const auto* entry = TexMan.FindEntry("star.png");
    return entry && entry->Valid;
}

void preview::drawShadow(d3d9::Device& device, unsigned elapsed)
{
    const auto duration = camera.m_dwTotalTime - camera.m_pKeyframes[0].TimeDelta;
    camera.Cerp(elapsed < duration ? elapsed : duration - 1);
    camera.UpdateViewMatrix(&device);
    D3DXMATRIXA16 projection;
    D3DXMatrixPerspectiveFovLH(&projection, D3DX_PI / 4, 4.f / 3, 1, 10000);
    device.SetTransform(D3DTS_PROJECTION, &projection);
    device.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device.SetRenderState(D3DRS_ZENABLE, TRUE);
    device.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device.Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff5f5f5f, 1, 0);

    D3DLIGHT9 light{};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse = {0.9f, 0.9f, 0.9f, 1};
    light.Specular = {0.1f, 0.1f, 0.1f, 1};
    light.Direction = {0, 0, 1};
    device.SetLight(0, &light);
    device.LightEnable(0, TRUE);
    device.SetRenderState(D3DRS_AMBIENT, 0xffffffff);
    device.SetRenderState(D3DRS_LIGHTING, TRUE);
    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    shadow.ClearTransform();
    shadow.SetTransform(TRANSFORM_TRANSLATE, 0, -1, 0);
    shadow.SetTransform(TRANSFORM_ROTATE, -D3DX_PI / 2, D3DX_PI, 0);
    shadow.SetTransform(TRANSFORM_SCALE, 300, 300, 300);
    shadow.Render(&device);

    device.SetRenderState(D3DRS_LIGHTING, FALSE);
    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    roomMaterial->Set(&device, nullptr);
    const float time = static_cast<float>(elapsed);
    roomMesh->Twist(0.1f, (time + 1000) / 30);
    roomMesh->ClearTransform();
    roomMesh->SetTransform(TRANSFORM_SCALE, 1000, 1000, 1000);
    roomMesh->Render(&device);

    device.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    device.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    const D3DXMATRIXA16 identity;
    device.SetTransform(D3DTS_WORLD, &identity);
    TexMan.SetTexture(&device, 0, "star.png");
    particles.ClearTransform();
    particles.SetTransform(TRANSFORM_SCALE, 9, 9, 9);
    particles.SetTransform(TRANSFORM_ROTATE, (time + 1000) / 600, time / 700, 0);
    particles.Render(&device);
    particles.ClearTransform();
    particles.SetTransform(TRANSFORM_SCALE, 9, 9, 9);
    particles.SetTransform(TRANSFORM_ROTATE, (time + 1000) / 700, time / 630, 0);
    particles.Render(&device);
    star.ClearTransform();
    star.Render(&device);
    device.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

void preview::releaseShadow()
{
    shadow.Release();
    room.Release();
    std::free(shadow.MaMa.MaterialList);
    std::free(room.MaMa.MaterialList);
    particles.Release();
    star.Release();
}
