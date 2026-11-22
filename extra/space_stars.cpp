#include "space_stars.h"
#include "mode.h"
#include "model.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <GL/glx.h>
#define APIENTRY
#endif
#include <GL/gl.h>

namespace {
struct Star { d3d9::Vector3 position; float brightness, warmth; };
struct Shell { std::vector<Star> stars; };
std::vector<Shell> shells;
const Scene3DS* owner;
GLuint starTexture;
using Activate = void (APIENTRY*)(GLenum);
Activate activeTexture()
{
#ifdef _WIN32
    return reinterpret_cast<Activate>(wglGetProcAddress("glActiveTexture"));
#else
    return reinterpret_cast<Activate>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glActiveTexture")));
#endif
}
void prepareStarTexture()
{
    if (starTexture) return;
    constexpr int size=16;
    unsigned char pixels[size*size*4];
    for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
        const float u=2*(x+0.5f)/size-1, v=2*(y+0.5f)/size-1;
        const int index=4*(y*size+x);
        pixels[index]=pixels[index+1]=pixels[index+2]=255;
        pixels[index+3]=static_cast<unsigned char>(255*std::clamp(4*(1-std::sqrt(u*u+v*v)),0.f,1.f));
    }
    glGenTextures(1,&starTexture);glBindTexture(GL_TEXTURE_2D,starTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
}
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
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (auto activate=activeTexture()) {
        GLint units=1;glGetIntegerv(0x84e2,&units);
        for (int unit=0;unit<units;++unit) {
            activate(0x84c0+unit);
            glDisable(GL_TEXTURE_1D);glDisable(GL_TEXTURE_2D);glDisable(0x806f);glDisable(0x8513);
            glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);glDisable(GL_TEXTURE_GEN_R);glDisable(GL_TEXTURE_GEN_Q);
        }
        activate(0x84c0);
    }
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(0x8513); // GL_TEXTURE_CUBE_MAP
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_FOG);
    prepareStarTexture();glBindTexture(GL_TEXTURE_2D,starTexture);glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glDepthMask(GL_FALSE);
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadMatrixf(&glProjection.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    for (unsigned layer=0; layer<shells.size(); ++layer) {
        const auto& shell = shells[layer];
        // Different apparent sizes keep the two moving layers readable as a starfield.
        const float size=std::max(1.f,(layer == 0 ? 3.2f : 2.1f)*viewport[3]/1080.f);
        const float scaleX=size/(viewport[2]*projection._11), scaleY=size/(viewport[3]*projection._22);
        glBegin(GL_QUADS);
        for (const auto& star : shell.stars) {
            const auto& p=star.position;
            const float x=p.x*modelView._11+p.y*modelView._21+p.z*modelView._31+modelView._41;
            const float y=p.x*modelView._12+p.y*modelView._22+p.z*modelView._32+modelView._42;
            const float z=p.x*modelView._13+p.y*modelView._23+p.z*modelView._33+modelView._43;
            if (z<=0) continue;
            const float halfX=z*scaleX, halfY=z*scaleY;
            if (std::abs(x)>z/projection._11+halfX || std::abs(y)>z/projection._22+halfY) continue;
            const float warm = star.warmth > 0.92f ? 1.f : 0.f;
            glColor4f(star.brightness*(0.7f+0.3f*warm),star.brightness*(0.84f+0.05f*warm),
                star.brightness*(1-0.25f*warm),1);
            glTexCoord2f(0,0);glVertex3f(x-halfX,y-halfY,z);
            glTexCoord2f(1,0);glVertex3f(x+halfX,y-halfY,z);
            glTexCoord2f(1,1);glVertex3f(x+halfX,y+halfY,z);
            glTexCoord2f(0,1);glVertex3f(x-halfX,y+halfY,z);
        }
        glEnd();
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_TEXTURE);glPopMatrix();
    glPopAttrib();
    return true;
}

void extra::releaseSpaceStars()
{
    if(starTexture)glDeleteTextures(1,&starTexture);
    starTexture=0;owner=nullptr;shells.clear();
}
