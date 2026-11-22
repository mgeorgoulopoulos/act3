#include "full_demo.h"
#include "space_preview.h"
#include "shadow_preview.h"
#include "blob_preview.h"
#include "credits_preview.h"
#include "shadow_blur.h"
#include "scene_host.h"
#include "eggplant_scene.h"
#include "extra/color_grade.h"
#include "extra/shadow_focus.h"
#include "extra/credits.h"
#include "extra/post_effect.h"
#include "extra/dramatometer.h"
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
struct Capture { GLuint texture{}; int width{},height{}; };
Capture spaceFrame,shadowFrame;
MeshGrid transitionGrid{};

void select(unsigned scene)
{
    TexMan.pFirstEntry=textures[scene];
    extra::creditsActive=scene==4;
}
void shadow(d3d9::Device& device,int width,int height,unsigned time)
{
    select(1);
    device.SetRenderState(d3d9::D3DRS_ZWRITEENABLE,TRUE);
    device.SetProjectionHorizontalScale(extra::enabled ? 4.f*height/(3.f*width) : 1);
    preview::drawShadow(device,time);
    device.SetProjectionHorizontalScale(1);
    extra::applyColorGrade(width,height,extra::SceneGrade::Shadow,time);
    extra::postProcessShadow(width,height,time);
}
void saveFrame(Capture& image,int width,int height)
{
    if (!image.texture) glGenTextures(1,&image.texture);
    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D,image.texture);
    if (width!=image.width || height!=image.height) {
        glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,0,0,width,height,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
        image.width=width; image.height=height;
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
    glPopAttrib();
}
void blendSpace(d3d9::Device& device,float alpha)
{
    device.SetTexture(1,nullptr); device.SetTexture(0,nullptr);
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); glDisable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,spaceFrame.texture);
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
void swirlShadow(d3d9::Device& device,int width,int height,float phase)
{
    device.SetTexture(1,nullptr); device.SetTexture(0,nullptr);
    device.SetRenderState(d3d9::D3DRS_LIGHTING,FALSE);
    device.SetRenderState(d3d9::D3DRS_ZENABLE,FALSE);
    device.SetRenderState(d3d9::D3DRS_ZWRITEENABLE,FALSE);
    device.SetRenderState(d3d9::D3DRS_CULLMODE,d3d9::D3DCULL_NONE);
    device.SetRenderState(d3d9::D3DRS_ALPHABLENDENABLE,TRUE);
    device.SetRenderState(d3d9::D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    device.SetRenderState(d3d9::D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device.SetTextureStageState(0,d3d9::D3DTSS_TEXCOORDINDEX,0);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,shadowFrame.texture);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    d3d9::Matrix identity,projection;
    const float aspect=float(width)/height;
    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4,aspect,1,10000);
    device.SetTransform(d3d9::D3DTS_VIEW,&identity);
    device.SetTransform(d3d9::D3DTS_PROJECTION,&projection);
    transitionGrid.ClearTransform();
    transitionGrid.SetTransform(TRANSFORM_TRANSLATE,0,0,11);
    transitionGrid.SetTransform(TRANSFORM_SCALE,aspect/(4.f/3.f),1,1);
    const unsigned value=static_cast<unsigned>(255*phase);
    transitionGrid.ApplyColor((value<<24)|(value<<16)|(value<<8)|value);
    // The original 32x32 grid, amplitude, interpolation and UV arithmetic.
    transitionGrid.VortexRectangle(1.5f*(1-phase));
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glTranslatef(0,1,0);glScalef(1,-1,1); // Copied GL framebuffers have bottom-up UVs.
    transitionGrid.Render(&device);
    device.SetRenderState(d3d9::D3DRS_ZWRITEENABLE,TRUE);
    glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(GL_MODELVIEW);
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
    transitionGrid.CreateRectangle(32,32,14,14*3/4);
    transitionGrid.CreateBuffers(&device);
    transitionGrid.RefreshBuffers();
    return true;
}
void preview::drawFullDemo(d3d9::Device& device,int width,int height,unsigned time)
{
    if (time<70000) {
        select(0); drawSpace(time);
        if (time>=69000) {
            saveFrame(spaceFrame,width,height);
            shadow(device,width,height,time-69000);
            saveFrame(shadowFrame,width,height);
            blendSpace(device,1);
            swirlShadow(device,width,height,(time-69000)/1000.f);
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
    extra::postProcess(width,height,time);
    extra::drawDramatometer(width,height,time);
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
    extra::releasePostProcess();
    glDeleteTextures(1,&spaceFrame.texture);glDeleteTextures(1,&shadowFrame.texture);
    spaceFrame={};shadowFrame={};
    if(transitionGrid.pVB) transitionGrid.Release();
    extra::creditsActive=false; fullDemoActive=false;
}
