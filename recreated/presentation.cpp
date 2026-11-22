#include "mesh_preview.h"
#include "../extra/mode.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <algorithm>
namespace { GLuint image;int imageWidth,imageHeight; }
void preview::present(int width,int height) {
    if(width<=0 || height<=0)return;
    const int aspectX=extra::enabled?16:4,aspectY=extra::enabled?9:3;
    int w=width,h=height;
    if(static_cast<long long>(width)*aspectY>static_cast<long long>(height)*aspectX)
        w=height*aspectX/aspectY;
    else h=width*aspectY/aspectX;
    if(w==width && h==height){draw(w,h);return;}
    draw(w,h);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if(!image)glGenTextures(1,&image);
    glBindTexture(GL_TEXTURE_2D,image);
    if(w!=imageWidth || h!=imageHeight){
        glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,0,0,w,h,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        imageWidth=w;imageHeight=h;
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,w,h);
    glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    glViewport((width-w)/2,(height-h)/2,w,h);
    glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);
    glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_TEXTURE);glPopMatrix();
    glPopAttrib();
}
void preview::releasePresentation(){if(image)glDeleteTextures(1,&image);image=0;imageWidth=imageHeight=0;}
