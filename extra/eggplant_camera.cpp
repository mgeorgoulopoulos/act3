#include "mode.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cmath>

namespace {
using d3d9::Vector3;
struct Pose { Vector3 eye, target; float roll; };
struct Shot { float end; Pose from, to; };
constexpr float pi = 3.14159265358979323846f;
constexpr unsigned harpStartMs = 29186;
constexpr unsigned harpEndMs = 35729;
const Shot shots[] = {
    // Pull back from the light and reveal the still figures below it.
    {8, {{975,298,0}, {1000,300,0}, 0}, {{-430,410,-760}, {390,210,-100}, 0}},
    // Cut to a low lateral track, with plants at several depths.
    {16, {{-500,115,-440}, {180,143,-170}, -3}, {{-435,150,475}, {230,145,120}, 3}},
    // Cross the line from the light to the egg at the origin, then uncover it.
    {24, {{-305,113,-140}, {360,205,-3}, -5}, {{-305,113,142}, {360,205,3}, 7}},
    // A wide, rolling rise into the cut to the recorded harp passage.
    {harpStartMs / 1000.f, {{283,105,842}, {275,180,210}, -13}, {{795,390,532}, {185,175,105}, 14}},
    // End inside the egg's silhouette for the cut to the rounded blob.
    {40, {{-32,208,-226}, {-10,156,-14}, -8}, {{-12,159,-46}, {-10,156,-14}, 0}},
};

Vector3 mix(Vector3 a, Vector3 b, float t)
{
    return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t};
}
Vector3 cross(Vector3 a, Vector3 b)
{
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
float dot(Vector3 a, Vector3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vector3 unit(Vector3 v)
{
    const float n = std::sqrt(dot(v,v));
    return {v.x/n, v.y/n, v.z/n};
}
}

bool extra::applyEggplantCamera(d3d9::Device& device, unsigned milliseconds)
{
    if (!enabled)
        return false;
    const auto time = milliseconds % 40000;
    // Keep the original rapid turns and roll at their recorded timestamps.
    if (time >= harpStartMs && time < harpEndMs)
        return false;
    const float seconds = time / 1000.f;
    float start = 0;
    for (const auto& shot : shots) {
        if (seconds >= shot.end) {
            start = shot.end;
            continue;
        }
        if (time >= harpEndMs)
            start = harpEndMs / 1000.f;
        const float phase = (seconds-start)/(shot.end-start);
        const float t = phase*phase*(3-2*phase);
        auto eye = mix(shot.from.eye, shot.to.eye, t);
        const auto target = mix(shot.from.target, shot.to.target, t);
        float roll = shot.from.roll+(shot.to.roll-shot.from.roll)*t;
        const auto forward = unit({target.x-eye.x,target.y-eye.y,target.z-eye.z});
        const auto right = unit(cross({0,1,0},forward));
        const auto up = cross(forward,right);
        const float angle = roll*pi/180;
        const float c = std::cos(angle), s = std::sin(angle);
        const auto r = mix({0,0,0}, right, c);
        const auto u = mix({0,0,0}, up, s);
        const Vector3 rolledRight{r.x+u.x,r.y+u.y,r.z+u.z};
        const Vector3 rolledUp = cross(forward,rolledRight);
        d3d9::Matrix view;
        view._11=rolledRight.x; view._21=rolledRight.y; view._31=rolledRight.z;
        view._12=rolledUp.x; view._22=rolledUp.y; view._32=rolledUp.z;
        view._13=forward.x; view._23=forward.y; view._33=forward.z;
        view._41=-dot(rolledRight,eye);
        view._42=-dot(rolledUp,eye);
        view._43=-dot(forward,eye);
        return device.SetTransform(d3d9::D3DTS_VIEW, &view) == d3d9::D3D_OK;
    }
    return false;
}
