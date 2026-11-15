#include "mode.h"
#include "../compatibility/d3d9/indexed_draw.h"
#include <cmath>

namespace {
using d3d9::Vector3;
struct Pose { Vector3 eye, target; float roll; };
struct Shot { float end; Pose from, to; };
constexpr float pi = 3.14159265358979323846f;
constexpr unsigned harpStartMs = 29186;
float viewportAspect = 4.f / 3.f;
const Shot shots[] = {
    // Pull back from the light and reveal the still figures below it.
    {8, {{975,298,0}, {1000,300,0}, 0}, {{-125,128,-195}, {595,172,-15}, 0}},
    // Cut to a low lateral track, with plants at several depths.
    {16, {{-500,115,-440}, {180,143,-170}, -3}, {{-435,150,475}, {230,145,120}, 3}},
    // Cross the line from the light to the egg at the origin, then uncover it.
    {24, {{-305,113,-140}, {360,205,-3}, -5}, {{-305,113,142}, {360,205,3}, 7}},
    // A wide, rolling rise into the cut to the recorded harp passage.
    {harpStartMs / 1000.f, {{283,105,842}, {275,180,210}, -13}, {{795,390,532}, {185,175,105}, 14}},
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

namespace {
bool applyCameraPose(d3d9::Device& device, unsigned milliseconds)
{
    const auto time = milliseconds % 40000;
    // Keep the original rapid turns and roll at their recorded timestamps.
    if (time >= harpStartMs)
        return false;
    const float seconds = time / 1000.f;
    float start = 0;
    for (const auto& shot : shots) {
        if (seconds >= shot.end) {
            start = shot.end;
            continue;
        }
        const float phase = (seconds-start)/(shot.end-start);
        const float t = phase*phase*(3-2*phase);
        auto eye = mix(shot.from.eye, shot.to.eye, t);
        if (start == 0) {
            // Drop early, then pass alongside the near row at stem height.
            const float descent = std::sin(pi * t);
            eye.y -= 100 * descent;
            eye.z -= 155 * descent;
        }
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
        // Blend into the recorded pose over the last two seconds of the wide shot.
        if (time >= harpStartMs - 2000) {
            d3d9::Matrix recorded;
            device.GetTransform(d3d9::D3DTS_VIEW, &recorded);
            const Vector3 recordedRight{recorded._11, recorded._21, recorded._31};
            const Vector3 recordedUp{recorded._12, recorded._22, recorded._32};
            const Vector3 recordedForward{recorded._13, recorded._23, recorded._33};
            const Vector3 recordedEye{
                -recorded._41*recordedRight.x-recorded._42*recordedUp.x-recorded._43*recordedForward.x,
                -recorded._41*recordedRight.y-recorded._42*recordedUp.y-recorded._43*recordedForward.y,
                -recorded._41*recordedRight.z-recorded._42*recordedUp.z-recorded._43*recordedForward.z};
            const float p = (time - (harpStartMs - 2000)) / 2000.f;
            const float weight = p*p*(3-2*p);
            const auto position = mix(eye, recordedEye, weight);
            const auto f = unit(mix(forward, recordedForward, weight));
            const auto r = unit(cross(mix(rolledUp, recordedUp, weight), f));
            const auto u = cross(f,r);
            view._11=r.x; view._21=r.y; view._31=r.z;
            view._12=u.x; view._22=u.y; view._32=u.z;
            view._13=f.x; view._23=f.y; view._33=f.z;
            view._41=-dot(r,position);
            view._42=-dot(u,position);
            view._43=-dot(f,position);
        }
        return device.SetTransform(d3d9::D3DTS_VIEW, &view) == d3d9::D3D_OK;
    }
    return false;
}

float eggplantFovScale(unsigned milliseconds)
{
    struct Cue { int time; float fov; };
    // Peaks align with measured transients in the 2:06 soundtrack excerpt.
    constexpr Cue cues[] = {{3460, 53}, {12680, 38}};
    for (const auto& cue : cues) {
        const float distance = static_cast<int>(milliseconds) - cue.time;
        const float phase = 1 - std::abs(distance) / (distance < 0 ? 360.f : 850.f);
        if (phase > 0) {
            const float amount = phase * phase * (3 - 2 * phase);
            const float fov = 45 + (cue.fov - 45) * amount;
            return std::tan(pi / 8) / std::tan(fov * pi / 360);
        }
    }
    return 1;
}
}

void extra::setEggplantViewport(int width, int height)
{
    viewportAspect = static_cast<float>(width) / height;
}

void extra::applyEggplantCamera(d3d9::Device& device, unsigned milliseconds,
    d3d9::Matrix& projection)
{
    if (!enabled)
        return;
    const float fovScale = eggplantFovScale(milliseconds);
    projection._11 *= fovScale * (4.f / 3.f) / viewportAspect;
    projection._22 *= fovScale;
    applyCameraPose(device, milliseconds);
}
