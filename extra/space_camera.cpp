#include "space_camera.h"
#include "mode.h"
#include "../compatibility/d3dx/d3dx9.h"
#include <algorithm>
#include <cmath>

namespace {
using d3d9::Vector3;
struct Pose { Vector3 eye, target; float fov, roll; };
struct Shot { unsigned start, end; Pose from, to; };
const Shot shots[] = {
    {0,6500,
        {{-15,30.2f,-392.5f},{-15,30.2f,-381.5f},38,0},
        {{-0.2f,30.2f,-392.5f},{-0.2f,30.2f,-381.5f},38,0}},
    // Cut from the engraving to a lateral portrait of the two raised figures.
    {6500,13000,
        {{6,32.3f,-394},{3.8f,31.8f,-390.5f},28,-3},
        {{2.1f,32.6f,-394},{3.5f,31.8f,-390.5f},30,2}},
    {13000,18500,
        {{1.7f,32,-393.2f},{2.8f,31.6f,-390.2f},26,2},
        {{2.3f,32.4f,-393},{3.2f,31.9f,-390.2f},29,-2}},
    // Inspect the Geiger instrument from outside the front of the body.
    {18500,26500,
        {{42,29,-425},{20.4f,21,-388.4f},32,-4},
        {{48,34,-420},{20.4f,21,-388.4f},35,2}},
    // A separate pass around the thermoelectric generator on its boom.
    {26500,34500,
        {{-125,93,-347},{-87,75,-302},35,3},
        {{-130,65,-340},{-87,75,-302},38,-3}},
    // Dish and feed assembly from beyond their bounding volume.
    {34500,42000,
        {{100,145,-475},{0,69,-375},40,-4},
        {{125,120,-460},{0,69,-375},44,0}},
};
Vector3 mix(Vector3 a, Vector3 b, float t)
{ return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t}; }
float smooth(float t) { return t*t*(3-2*t); }
Vector3 cross(Vector3 a, Vector3 b)
{ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
Vector3 unit(Vector3 a)
{ Vector3 result; D3DXVec3Normalize(&result,&a); return result; }
float dot(Vector3 a,Vector3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
}

void extra::applySpaceCamera(d3d9::Device& device, unsigned time, d3d9::Matrix& projection)
{
    if (!enabled || time >= 61000)
        return;
    Pose pose = shots[0].from;
    for (const auto& shot : shots) {
        if (time >= shot.end)
            continue;
        const float t = smooth(float(time-shot.start)/(shot.end-shot.start));
        pose = {mix(shot.from.eye,shot.to.eye,t), mix(shot.from.target,shot.to.target,t),
            shot.from.fov+(shot.to.fov-shot.from.fov)*t,
            shot.from.roll+(shot.to.roll-shot.from.roll)*t};
        break;
    }
    if (time >= 42000) {
        const float phase = std::min(1.f, (time-42000)/13500.f);
        const float angle = -1.8f + 1.5f*phase;
        constexpr float radius = 330.f;
        pose = {{radius*std::cos(angle),100+25*std::sin(phase*3.14159265f),
            -360+radius*std::sin(angle)}, {0,65,-360}, 48, 2*std::sin(phase*3.14159265f)};
    }
    auto forward = unit({pose.target.x-pose.eye.x,pose.target.y-pose.eye.y,pose.target.z-pose.eye.z});
    auto right = unit(cross({0,1,0},forward));
    auto up = cross(forward,right);
    const float roll = pose.roll * 0.01745329252f;
    right = mix({0,0,0},right,std::cos(roll));
    right = unit({right.x+up.x*std::sin(roll),right.y+up.y*std::sin(roll),right.z+up.z*std::sin(roll)});
    up = cross(forward,right);
    if (time > 55500) {
        d3d9::Matrix recorded;
        device.GetTransform(d3d9::D3DTS_VIEW,&recorded);
        const Vector3 rr{recorded._11,recorded._21,recorded._31};
        const Vector3 ru{recorded._12,recorded._22,recorded._32};
        const Vector3 rf{recorded._13,recorded._23,recorded._33};
        const Vector3 eye{-recorded._41*rr.x-recorded._42*ru.x-recorded._43*rf.x,
            -recorded._41*rr.y-recorded._42*ru.y-recorded._43*rf.y,
            -recorded._41*rr.z-recorded._42*ru.z-recorded._43*rf.z};
        const float t = smooth((time-55500)/5500.f);
        pose.eye = mix(pose.eye,eye,t);
        forward = unit(mix(forward,rf,t));
        right = unit(cross(unit(mix(up,ru,t)),forward));
        up = cross(forward,right);
        pose.fov += (45-pose.fov)*t;
    }
    d3d9::Matrix view;
    view._11=right.x; view._21=right.y; view._31=right.z;
    view._12=up.x; view._22=up.y; view._32=up.z;
    view._13=forward.x; view._23=forward.y; view._33=forward.z;
    view._41=-dot(right,pose.eye); view._42=-dot(up,pose.eye); view._43=-dot(forward,pose.eye);
    device.SetTransform(d3d9::D3DTS_VIEW,&view);
    // A shorter near plane permits the close inspection of the plaque figures.
    D3DXMatrixPerspectiveFovLH(&projection,pose.fov*0.01745329252f,4.f/3.f,0.05f,10000);
    if (time < 6500) {
        // Parallel rays keep the plaque and raised figures scrolling as one flat image.
        d3d9::Matrix flat;
        constexpr float halfHeight = 7.4f;
        flat._11 = 1.f / (halfHeight * 4.f / 3.f);
        flat._22 = 1.f / halfHeight;
        flat._33 = 1.f / 10000.f;
        flat._43 = -0.05f / 10000.f;
        const float blend = 0;
        const Vector3 delta{pose.target.x-pose.eye.x,pose.target.y-pose.eye.y,pose.target.z-pose.eye.z};
        const float focusDistance = std::sqrt(dot(delta,delta));
        // Normalize the homogeneous scale at the subject before mixing projection types.
        for (int row=0; row<4; ++row)
            for (int column=0; column<4; ++column)
                projection.m[row][column] = flat.m[row][column] * (1-blend)
                    + projection.m[row][column] * blend / focusDistance;
    }
}
