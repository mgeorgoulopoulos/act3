#include "mesh_preview.h"
#include "compatibility/d3dx/d3dx9.h"
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>

void preview::drawLoading(int width,int height)
{
    glViewport(0,0,width,height);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    d3d9::Device device;
    LPDIRECT3DTEXTURE9 image=nullptr;
    if(D3DXCreateTextureFromFileEx(&device,"data/textures/loading.png",0,0,D3DX_DEFAULT,0,
        D3DFMT_DXT1,D3DPOOL_DEFAULT,D3DX_DEFAULT,D3DX_DEFAULT,0,nullptr,nullptr,&image)<0) return;
    const float edge=static_cast<float>(std::min(width,height));
    const float x=(width-edge)/2,y=(height-edge)/2;
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,image->name);glColor3f(1,1,1);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+edge,y);
    glTexCoord2f(1,1);glVertex2f(x+edge,y+edge);glTexCoord2f(0,1);glVertex2f(x,y+edge);glEnd();
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();image->Release();
}
