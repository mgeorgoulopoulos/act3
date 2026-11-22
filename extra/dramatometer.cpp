#include "dramatometer.h"
#include "post_effect.h"
#include "mode.h"
#include "../recreated/music.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <GL/glx.h>
#endif
#include <GL/gl.h>

namespace {
bool visible{},held{};
unsigned presses{};
void rectangle(float x,float y,float w,float h)
{
    glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
// Built-in 5x7 glyphs keep the hidden overlay independent of OS font APIs.
const unsigned char* glyph(char c)
{
    static const unsigned char alphabet[][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
    static const unsigned char numbers[][7]={
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}};
    static const unsigned char percent[7]={17,2,4,8,17,0,0},blank[7]={};
    if(c>='A'&&c<='Z')return alphabet[c-'A'];
    if(c>='0'&&c<='9')return numbers[c-'0'];
    return c=='%'?percent:blank;
}
void text(float x,float y,const char* label,float size)
{
    for(;*label;++label,x+=6*size) {
        const auto bits=glyph(*label);
        for(int row=0;row<7;++row)for(int col=0;col<5;++col)
            if(bits[row]&(1<<(4-col))) rectangle(x+col*size,y+row*size,size,size);
    }
}
}
bool extra::dramatometerActive() { return visible; }

void extra::dramatometerKey(bool control,bool isD,bool pressed)
{
    if(!control){presses=0;held=false;return;}
    if(!isD)return;
    if(!pressed){held=false;return;}
    if(held)return;
    held=true;
    if(++presses==5){visible=!visible;presses=0;}
}
void extra::drawDramatometer(int width,int height,unsigned time)
{
    if(!visible)return;
    const float scale=std::clamp(height/720.f,1.f,2.f);
    const float base=std::max(0.1f,dramaAt(time));
    const float pulse=music::features().orchestralPulse;
    const float final=postIntensity(time);
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
#ifdef _WIN32
    using Active=void (WINAPI*)(GLenum);
    auto active=reinterpret_cast<Active>(wglGetProcAddress("glActiveTexture"));
#else
    using Active=void (*)(GLenum);
    auto active=reinterpret_cast<Active>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glActiveTexture")));
#endif
    if(active){active(0x84c1);glDisable(GL_TEXTURE_2D);glDisable(0x8513);active(0x84c0);}
    glDisable(GL_TEXTURE_2D);glDisable(0x8513);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glScalef(scale,scale,1);
    glTranslatef(0,height/scale-188,0);
    glColor4f(0.015f,0.02f,0.035f,0.90f);rectangle(16,16,316,156);
    glColor4f(0.6f,0.9f,1,1);text(28,28,"DRAMATOMETER",2);
    const float values[]={base,pulse,final};const char* labels[]={"BASE","MUSIC","FINAL"};
    for(int row=0;row<3;++row){
        const float y=58.f+36*row;char label[40];std::snprintf(label,sizeof(label),"%s %d%%",labels[row],int(values[row]*100+0.5f));
        glColor4f(0.9f,0.93f,1,1);text(28,y,label,1.5f);
        glColor4f(0.12f,0.15f,0.2f,1);rectangle(28,y+14,288,8);
        glColor4f(row==2?1.f:0.25f,row==2?0.55f:0.8f,row==2?0.25f:1.f,1);
        const float ceiling=row==1?1.f:2.f;
        rectangle(28,y+14,288*std::clamp(values[row]/ceiling,0.f,1.f),8);
        if(row!=1){glColor4f(0.8f,0.85f,0.9f,1);rectangle(171,y+13,1,10);}
    }
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();
}
