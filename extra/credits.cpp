#include "credits.h"
#include "mode.h"
#include "model.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

void extra::layoutCredits(MeshGrid& names)
{
    if (enabled) {
        // Keep the artwork's 475:186 aspect on the historical 100:60 surface.
        D3DXMATRIXA16 scale;
        D3DXMatrixScaling(&scale,1,100.f*186.f/(60.f*475.f),1);
        names.Transform = scale*names.Transform;
    }
}

void extra::layoutCreditsBackground(MeshGrid& background)
{
    if (!enabled) return;
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    const float aspect = float(viewport[2])/viewport[3];
    if (aspect <= 4.f/3.f) return;
    D3DXMATRIXA16 scale;
    D3DXMatrixScaling(&scale,aspect/(4.f/3.f),1,1);
    background.Transform = scale*background.Transform;
}

bool extra::drawCreditsCard(unsigned long color, float sourceAspect)
{
    if (!enabled) return false;
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    const float aspect = float(viewport[2])/viewport[3];
    // Fill the frame, keeping the image's aspect and cropping its outer margins.
    const float u = aspect < sourceAspect ? aspect/sourceAspect : 1.f;
    const float v = aspect > sourceAspect ? sourceAspect/aspect : 1.f;
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); glDisable(GL_TEXTURE_GEN_R);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,1,1,0,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4ub((color>>16)&255,(color>>8)&255,color&255,(color>>24)&255);
    glBegin(GL_QUADS);
    glTexCoord2f((1-u)/2,(1-v)/2); glVertex2f(0,0);
    glTexCoord2f((1+u)/2,(1-v)/2); glVertex2f(1,0);
    glTexCoord2f((1+u)/2,(1+v)/2); glVertex2f(1,1);
    glTexCoord2f((1-u)/2,(1+v)/2); glVertex2f(0,1);
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glPopAttrib();
    return true;
}
