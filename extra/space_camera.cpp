#include "space_camera.h"
#include "mode.h"
#include "../compatibility/d3dx/d3dx9.h"
#include <algorithm>
#include <cmath>

namespace {
using d3d9::Vector3;
struct Pose { Vector3 eye, target; float fov, roll; };
struct Key { unsigned time; Pose pose; };
const Key keys[] = {
    {0,     {{-2.4f,30.2f,-400}, {-2.4f,30.2f,-389}, 38, 0}},
    {6500,  {{0.6f,30.2f,-400}, {0.6f,30.2f,-389}, 38, 0}},
    {15000, {{7.0f,32.2f,-394}, {3.7f,31.9f,-390.5f}, 24, -3}},
    {23000, {{4.5f,32.5f,-392.8f}, {3.5f,31.9f,-390.5f}, 32, 2}},
    {30000, {{-7,36,-398}, {0,34,-389}, 38, -3}},
    {40500, {{65,65,-505}, {0,45,-375}, 43, 0}},
    {51000, {{125,75,-365}, {0,45,-375}, 43, 4}},
    {55500, {{85,65,-560}, {0,45,-375}, 43, 0}},
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
    Pose pose = keys[0].pose;
    for (unsigned i=1; i<sizeof(keys)/sizeof(keys[0]); ++i) {
        const auto& from = keys[i-1];
        const auto& to = keys[i];
        const float t = smooth(std::clamp(float(time-from.time)/(to.time-from.time),0.f,1.f));
        pose = {mix(from.pose.eye,to.pose.eye,t), mix(from.pose.target,to.pose.target,t),
            from.pose.fov+(to.pose.fov-from.pose.fov)*t,
            from.pose.roll+(to.pose.roll-from.pose.roll)*t};
        if (time < to.time)
            break;
    }
    if (time >= 40500) {
        const float phase = std::min(1.f, (time-40500)/15000.f);
        const float angle = -1.10714872f + 1.55f*phase;
        const float radius = 145.34f + 12.f*phase;
        pose.eye = {radius*std::cos(angle), 65+10*std::sin(phase*3.14159265f),
            -375+radius*std::sin(angle)};
        pose.target = {0,45,-375};
        pose.fov = 43;
        pose.roll = 3*std::sin(phase*3.14159265f);
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
    if (time < 11500) {
        // Parallel rays keep the plaque and raised figures scrolling as one flat image.
        d3d9::Matrix flat;
        constexpr float halfHeight = 7.4f;
        flat._11 = 1.f / (halfHeight * 4.f / 3.f);
        flat._22 = 1.f / halfHeight;
        flat._33 = 1.f / 10000.f;
        flat._43 = -0.05f / 10000.f;
        const float blend = smooth(std::clamp((float(time)-6500)/5000.f,0.f,1.f));
        const Vector3 delta{pose.target.x-pose.eye.x,pose.target.y-pose.eye.y,pose.target.z-pose.eye.z};
        const float focusDistance = std::sqrt(dot(delta,delta));
        // Normalize the homogeneous scale at the subject before mixing projection types.
        for (int row=0; row<4; ++row)
            for (int column=0; column<4; ++column)
                projection.m[row][column] = flat.m[row][column] * (1-blend)
                    + projection.m[row][column] * blend / focusDistance;
    }
}
