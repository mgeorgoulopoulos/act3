#include "shadow_preview.h"
#include "scene_host.h"
#include "camera.h"
#include <cstdlib>

namespace {
Scene3DS shadow{}, room{};
CameraSequence camera;
}

bool preview::loadShadow(d3d9::Device& device)
{
    return shadow.LoadFromS3D("data/geometry/shadow.s3d", &device) &&
        room.LoadFromS3D("data/geometry/room.s3d", &device) &&
        camera.Load("data/geometry/shadow.cam") && camera.m_dwNumShots >= 2 &&
        camera.m_dwTotalTime > camera.m_pKeyframes[0].TimeDelta;
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
    room.ClearTransform();
    room.SetTransform(TRANSFORM_SCALE, 1000, 1000, 1000);
    room.Render(&device);
}

void preview::releaseShadow()
{
    shadow.Release();
    room.Release();
    std::free(shadow.MaMa.MaterialList);
    std::free(room.MaMa.MaterialList);
}
