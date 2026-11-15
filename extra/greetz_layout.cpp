#include "greetz_layout.h"
#include "mode.h"
#include "model.h"
#include "TextureManager.h"
#include <cmath>

extern TextureManager TexMan;

bool extra::drawGreetz(d3d9::Device& device, MeshGrid& grid, unsigned time)
{
    if (!enabled)
        return false;

    struct Card {
        const char* texture;
        float x, y;
        unsigned period, offset;
    };
    const Card cards[] = {
        {"Greetz/Asd.png",       -0.68f,  0.71f, 4000,  200},
        {"Greetz/TheLab.png",    -0.64f,  0.38f, 5000,  850},
        {"Greetz/deus.png",       0.65f,  0.70f, 3000, 1200},
        {time < 6000 ? "Greetz/Madwizards.png" : "Greetz/Conspiracy.png",
                                 -0.62f, -0.65f, 6000,    0},
        {"Greetz/SenseAmok.png",   0.64f, -0.55f, 7000, 1900},
    };

    // The historical overlays already have a centered camera 300 units away.
    d3d9::Matrix projection;
    device.GetTransform(d3d9::D3DTS_PROJECTION, &projection);
    const float halfWidth = 300.f / projection._11;
    const float halfHeight = 300.f / projection._22;
    constexpr float fullTurn = 6.28318530718f;
    for (const auto& card : cards) {
        const float phase = fullTurn * ((time + card.offset) % card.period) / card.period;
        const float pulse = 0.5f - 0.5f * std::cos(phase);
        const float direction = card.x < 0 ? -1.f : 1.f;
        const float x = card.x + direction * (0.08f - 0.13f * pulse);
        const float y = card.y + 0.035f * std::sin(phase);
        const float scale = halfHeight * (0.42f + 0.09f * pulse) / 64.f;
        const float exit = card.period == 6000 && time >= 11400
            ? (time < 12000 ? (12000 - time) / 600.f : 0.f) : 1.f;
        const auto tint = static_cast<unsigned>(exit * (70 + 185 * pulse));
        if (!tint)
            continue;

        TexMan.SetTexture(&device, 0, card.texture);
        grid.ApplyColor(0xff000000u | tint * 0x010101u);
        grid.RefreshBuffers();
        grid.ClearTransform();
        grid.SetTransform(TRANSFORM_SCALE, scale, scale, 1);
        grid.SetTransform(TRANSFORM_TRANSLATE, x * halfWidth, y * halfHeight, 0);
        grid.Render(&device);
    }
    return true;
}
