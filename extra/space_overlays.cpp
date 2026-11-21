#include "space_overlays.h"
#include "mode.h"
#include "model.h"
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

void extra::prepareSpaceOverlays(d3d9::Device& device)
{
    if (!enabled)
        return;
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
    glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_CURRENT_BIT);
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
    glColor4ub((color>>16)&255,(color>>8)&255,color&255,color>>24);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(left,top);
    glTexCoord2f(1,0); glVertex2f(left+width,top);
    glTexCoord2f(1,1); glVertex2f(left+width,top+height);
    glTexCoord2f(0,1); glVertex2f(left,top+height);
    glEnd();
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
