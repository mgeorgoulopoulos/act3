#include "space_stars.h"
#include "mode.h"
#include "model.h"
#include <cmath>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

namespace {
struct Star { d3d9::Vector3 position; float brightness, warmth; };
struct Shell { std::vector<Star> stars; };
std::vector<Shell> shells;
const Scene3DS* owner;
float random(std::uint32_t& seed)
{
    seed = seed*1664525u+1013904223u;
    return float(seed>>8)/16777216.f;
}
void initialize(Scene3DS& scene)
{
    shells.clear();
    owner = &scene;
    for (unsigned object=0; object<scene.NumObjects; ++object) {
        const auto& mesh = scene.ObjectList[object];
        if (!mesh.NumVertices) {
            shells.push_back({});
            continue;
        }
        d3d9::Vector3 low = {mesh.VertexList[0].x,mesh.VertexList[0].y,mesh.VertexList[0].z};
        auto high = low;
        for (unsigned i=1; i<mesh.NumVertices; ++i) {
            const auto& v = mesh.VertexList[i];
            if(v.x<low.x) low.x=v.x;
            if(v.y<low.y) low.y=v.y;
            if(v.z<low.z) low.z=v.z;
            if(v.x>high.x) high.x=v.x;
            if(v.y>high.y) high.y=v.y;
            if(v.z>high.z) high.z=v.z;
        }
        const d3d9::Vector3 center{(low.x+high.x)/2,(low.y+high.y)/2,(low.z+high.z)/2};
        const d3d9::Vector3 radius{(high.x-low.x)/2,(high.y-low.y)/2,(high.z-low.z)/2};
        Shell shell;
        std::uint32_t seed = 18473+object*673;
        constexpr unsigned count = 10000;
        shell.stars.reserve(count);
        for (unsigned i=0; i<count; ++i) {
            // Uniform sphere sampling has no UV seam or polar concentration.
            const float y = 2*random(seed)-1;
            const float angle = 6.283185307f*random(seed);
            const float ring = std::sqrt(1-y*y);
            const d3d9::Vector3 p{center.x+radius.x*ring*std::cos(angle),
                center.y+radius.y*y,center.z+radius.z*ring*std::sin(angle)};
            const float intensity = random(seed);
            shell.stars.push_back({p,0.34f+0.66f*intensity*intensity,random(seed)});
        }
        shells.push_back(std::move(shell));
    }
}
}

bool extra::drawSpaceStars(d3d9::Device& device, Scene3DS& scene)
{
    if (!enabled)
        return false;
    if (owner != &scene || shells.size() != scene.NumObjects)
        initialize(scene);

    d3d9::Matrix view, projection, clip;
    device.GetTransform(d3d9::D3DTS_VIEW,&view);
    device.GetTransform(d3d9::D3DTS_PROJECTION,&projection);
    clip._33 = 2; clip._43 = -1;
    const auto glProjection = projection*clip;
    const auto modelView = scene.Transform*view;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POINT_BIT | GL_TRANSFORM_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(0x8513); // GL_TEXTURE_CUBE_MAP
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_POINT_SMOOTH);
    glDepthMask(GL_FALSE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadMatrixf(&glProjection.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadMatrixf(&modelView.m[0][0]);
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    for (unsigned layer=0; layer<shells.size(); ++layer) {
        const auto& shell = shells[layer];
        // Different apparent sizes keep the two moving layers readable as a starfield.
        glPointSize((layer == 0 ? 3.2f : 2.1f)*viewport[3]/1080.f);
        glBegin(GL_POINTS);
        for (const auto& star : shell.stars) {
            const float warm = star.warmth > 0.92f ? 1.f : 0.f;
            glColor4f(star.brightness*(0.7f+0.3f*warm),star.brightness*(0.84f+0.05f*warm),
                star.brightness*(1-0.25f*warm),1);
            glVertex3f(star.position.x,star.position.y,star.position.z);
        }
        glEnd();
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glPopAttrib();
    return true;
}
