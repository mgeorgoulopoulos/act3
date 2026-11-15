#include "greetz_layout.h"
#include "mode.h"
#include "model.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

extern TextureManager TexMan;

bool extra::drawGreetz(d3d9::Device& device, MeshGrid& grid, unsigned time, bool feedback)
{
    using namespace d3d9;
    if (!enabled)
        return false;

    struct Card {
        const char* texture;
        float x, y;
        unsigned entry, period;
    };
    constexpr Card cards[] = {
        {"Greetz/Asd.png",        -0.68f,  0.71f,     0, 3907},
        {"Greetz/TheLab.png",     -0.64f,  0.38f,   950, 4871},
        {"Greetz/deus.png",        0.65f,  0.70f,  1900, 3137},
        {"Greetz/Madwizards.png", -0.62f, -0.65f,  2850, 5323},
        {"Greetz/SenseAmok.png",   0.64f, -0.55f,  3800, 6173},
        {"Greetz/Conspiracy.png", -0.62f, -0.65f,  6300, 5689},
    };

    Matrix projection;
    device.GetTransform(D3DTS_PROJECTION, &projection);
    if (feedback) {
        Matrix view;
        view._43 = 300.f;
        device.SetTransform(D3DTS_VIEW, &view);
        device.SetRenderState(D3DRS_ZENABLE, FALSE);
        device.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        device.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    }

    const float halfWidth = 300.f / projection._11;
    const float halfHeight = 300.f / projection._22;
    constexpr float fullTurn = 6.28318530718f;
    for (const auto& card : cards) {
        if (time < card.entry || (card.entry == 2850 && time >= 6300))
            continue;

        const auto age = time - card.entry;
        const float phase = fullTurn * (age % card.period) / card.period;
        const float wave = 0.5f - 0.5f * std::cos(phase + 2.1f);
        const float entrance = std::min(1.f, age / 420.f);
        const float emphasis = wave * wave;
        const float direction = card.x < 0 ? -1.f : 1.f;
        const float x = card.x + direction * (0.10f - 0.15f * emphasis);
        const float y = card.y + 0.045f * std::sin(phase);
        const float scale = halfHeight * 0.47f / 64.f;
        const float squeeze = 0.08f + 0.92f * emphasis;
        const float brightness = (0.18f + 0.82f * emphasis) * entrance * (feedback ? 0.32f : 1.f);
        const auto tint = static_cast<unsigned>(255 * brightness);

        TexMan.SetTexture(&device, 0, card.texture);
        grid.ApplyColor(0xff000000u | tint * 0x010101u);
        grid.RefreshBuffers();
        grid.ClearTransform();
        grid.SetTransform(TRANSFORM_SCALE, scale * squeeze, scale * (1.6f - 0.6f * squeeze), 1);
        grid.SetTransform(TRANSFORM_TRANSLATE, x * halfWidth, y * halfHeight, 0);
        grid.Render(&device);
    }

    if (feedback) {
        device.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device.SetRenderState(D3DRS_ZENABLE, TRUE);
    }
    return true;
}
