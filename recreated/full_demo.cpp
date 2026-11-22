#include "full_demo.h"
#include "space_preview.h"
#include "shadow_preview.h"
#include "blob_preview.h"
#include "credits_preview.h"
#include "shadow_blur.h"
#include "scene_host.h"
#include "../EggplantScene.h"
#include "extra/color_grade.h"
#include "extra/shadow_focus.h"
#include "extra/credits.h"
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

extern TextureManager TexMan;
namespace {
Scene1 eggplants{};
DemoEngine host{};
TextureEntry* textures[5]{};
bool loaded[5]{};
GLuint spaceFrame{};
int frameWidth{},frameHeight{};

void select(unsigned scene)
{
    TexMan.pFirstEntry=textures[scene];
    extra::creditsActive=scene==4;
}
void shadow(d3d9::Device& device,int width,int height,unsigned time)
{
    select(1);
    device.SetProjectionHorizontalScale(extra::enabled ? 4.f*height/(3.f*width) : 1);
    preview::drawShadow(device,time);
    device.SetProjectionHorizontalScale(1);
    extra::applyColorGrade(width,height,extra::SceneGrade::Shadow,time);
    extra::postProcessShadow(width,height,time);
}
void saveSpace(int width,int height)
{
    if (!spaceFrame) glGenTextures(1,&spaceFrame);
    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D,spaceFrame);
    if (width!=frameWidth || height!=frameHeight) {
        glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,0,0,width,height,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
        frameWidth=width; frameHeight=height;
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
    glPopAttrib();
}
void blendSpace(d3d9::Device& device,float alpha)
{
    device.SetTexture(1,nullptr); device.SetTexture(0,nullptr);
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); glDisable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,spaceFrame);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(1,1,1,alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(-1,-1); glTexCoord2f(1,0); glVertex2f(1,-1);
    glTexCoord2f(1,1); glVertex2f(1,1); glTexCoord2f(0,1); glVertex2f(-1,1);
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glPopAttrib();
}
}

bool preview::loadFullDemo(d3d9::Device& device)
{
    fullDemoActive=true;
    // Each original scene keeps its own texture list, so identically named art
    // can have different Extra versions without changing TextureManager.cpp.
    for (unsigned i=0;i<5;++i) {
        TexMan.pFirstEntry=nullptr; extra::creditsActive=false;
        bool ready=false;
        switch(i) {
        case 0: ready=loadSpace(device); break;
        case 1: ready=loadShadow(device); break;
        case 2:
            host.m_pd3dDevice=&device; eggplants.m_pHostDemo=&host;
            eggplants.m_dwDuration=40000; ready=eggplants.Init(); break;
        case 3: ready=loadBlob(device); break;
        case 4: ready=loadCredits(device); break;
        }
        textures[i]=TexMan.pFirstEntry;
        loaded[i]=true;
        if (!ready) return false;
    }
    TexMan.pFirstEntry=nullptr; extra::creditsActive=false;
    return true;
}
void preview::drawFullDemo(d3d9::Device& device,int width,int height,unsigned time)
{
    if (time<70000) {
        select(0); drawSpace(time);
        if (time>=69000) {
            saveSpace(width,height);
            shadow(device,width,height,time-69000);
            blendSpace(device,1-(time-69000)/1000.f);
        }
    } else if (time<126000) shadow(device,width,height,time-70000);
    else if (time<166000) {
        select(2); eggplants.m_dwTime=time-126000; eggplants.Render();
        extra::applyColorGrade(width,height,extra::SceneGrade::Eggplants);
    } else if (time<185300) {
        select(3); drawBlob(device,width,height,time-166000);
    } else if (time<fullDemoDuration) {
        select(4); drawCredits(device,width,height,time-185300);
    } else device.Clear(0,nullptr,1,0xff000000,1,0);
}
void preview::releaseFullDemo()
{
    for (unsigned i=0;i<5;++i) if (loaded[i]) {
        select(i);
        switch(i) {
        case 0: releaseSpace(); break;
        case 1: releaseShadow(); TexMan.Release(); break;
        case 2:
            eggplants.Release();
            if(eggplants.CubeMap) eggplants.CubeMap->Release();
            std::free(eggplants.scene3d.MaMa.MaterialList);
            std::free(eggplants.floor1.MaMa.MaterialList);
            TexMan.Release(); break;
        case 3: releaseBlob(); break;
        case 4: releaseCredits(); break;
        }
        loaded[i]=false; textures[i]=nullptr; TexMan.pFirstEntry=nullptr;
    }
    extra::releaseColorGrade(); recreated::releaseShadowBlur(); extra::releaseShadowFocus();
    glDeleteTextures(1,&spaceFrame); spaceFrame=0; frameWidth=frameHeight=0;
    extra::creditsActive=false; fullDemoActive=false;
}
