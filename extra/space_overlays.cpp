#include "space_overlays.h"
#include "mode.h"
#include "color_grade.h"
#include "model.h"
#include "TextureManager.h"
#include <cmath>
extern TextureManager TexMan;
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
namespace { unsigned definitionTime; }

void extra::prepareSpaceOverlays(d3d9::Device& device, unsigned time)
{
    if (!enabled)
        return;
    definitionTime = time;
    // Grade the rendered universe before the artist's introductory overlays.
    if (time >= 15000) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT,viewport);
        applyColorGrade(viewport[2],viewport[3],SceneGrade::Space);
    }
    if (time < 15000)
        device.Clear(0,nullptr,1,0xff000000,1,0);
    device.SetTexture(1,nullptr);
    device.SetRenderState(d3d9::D3DRS_LIGHTING,FALSE);
    device.SetRenderState(d3d9::D3DRS_ZENABLE,FALSE);
    device.SetRenderState(d3d9::D3DRS_ZWRITEENABLE,FALSE);
    device.SetRenderState(d3d9::D3DRS_ALPHABLENDENABLE,TRUE);
    device.SetTextureStageState(0,d3d9::D3DTSS_TEXCOORDINDEX,0);
    d3d9::Matrix projection;
    // Device applies the viewport's widescreen correction to this 4:3 reference lens.
    D3DXMatrixPerspectiveFovLH(&projection,0.7853981634f,4.f/3.f,1,10000);
    device.SetTransform(d3d9::D3DTS_PROJECTION,&projection);
}

bool extra::drawSpaceDefinition(unsigned long color)
{
    if (!enabled)
        return false;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    const float aspect = float(viewport[2])/viewport[3];
    const float width = std::min(1.f,(4.f/3.f)/aspect);
    const float height = std::min(1.f,aspect/(4.f/3.f));
    const float left = (1-width)/2, top = (1-height)/2;
    glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,1,1,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    const auto quad = [&](float x, float y) {
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(left+x,top+y);
        glTexCoord2f(1,0); glVertex2f(left+width+x,top+y);
        glTexCoord2f(1,1); glVertex2f(left+width+x,top+height+y);
        glTexCoord2f(0,1); glVertex2f(left+x,top+height+y);
        glEnd();
    };
    const float fade = ((color>>24)&255)/255.f;
    // Retain the opaque black page and the historical one-second dissolve.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glColor4f(0,0,0,fade);
    quad(0,0);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    const float seconds = definitionTime/1000.f;
    const float breath = 0.88f + 0.09f*std::sin(seconds*1.7f)
        + 0.03f*std::sin(seconds*4.3f);
    // Small additive copies of the HD mask make a soft red/orange backlight.
    // Radius follows the fitted page, so it looks the same at every resolution.
    for (int ring=1;ring<=3;++ring) {
        const float radius = ring*0.0015f*(1+0.10f*std::sin(seconds*1.3f));
        glColor4f(1,0.12f,0.015f,fade*breath*(0.16f/ring));
        for (int sample=0;sample<12;++sample) {
            const float angle = sample*6.2831853f/12;
            quad(std::cos(angle)*radius*width*0.75f,std::sin(angle)*radius*height);
        }
    }
    // The sharp gold core stays steady and readable while its halo breathes.
    glColor4f(1,0.84f,0.08f,fade*(0.94f+0.06f*breath));
    quad(0,0);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glPopAttrib();
    return true;
}

void extra::layoutSpaceQuote(MeshGrid& grid)
{
    if (enabled)
        grid.SetTransform(TRANSFORM_SCALE,1,1.f/3.f,1);
}


bool extra::drawSpaceFire(d3d9::Device& device, d3d9::BaseTexture* texture, unsigned time)
{
    if (!enabled) return false;

    device.SetTexture(1,nullptr);
    device.SetTexture(0,texture);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0,1,1,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    const auto quad = [](float y, float height) {
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(0,y);
        glTexCoord2f(1,0); glVertex2f(1,y);
        glTexCoord2f(1,1); glVertex2f(1,y+height);
        glTexCoord2f(0,1); glVertex2f(0,y+height);
        glEnd();
    };
    // Preserve the historical two-pass blending, including its blue zero-alpha texels.
    glColor4f(1,1,1,1);
    quad(0,1);
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA);
    quad(0,1);
    if (time < 33000) {
        TexMan.SetTexture(&device,0,"Noise.jpg");
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(0.53f,0.53f,0.53f,1);
        quad((std::sin(time/1000.f)+1)/2,0.10f);
        quad((std::sin(time/1100.f)+1)/2,0.05f);
        quad((std::sin(time/700.f)+1)/2,0.17f);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glPopAttrib();
    return true;
}
