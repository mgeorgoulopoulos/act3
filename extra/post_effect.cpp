#include "post_effect.h"
#include "mode.h"
#include "../recreated/music.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
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
float ease(unsigned time,unsigned start,unsigned end)
{ const float t=std::clamp((float(time)-start)/(end-start),0.f,1.f); return t*t*(3-2*t); }
GLuint program{},frame{};
int frameWidth{},frameHeight{};
bool attempted{};
using Object=void (APIENTRY*)(GLuint);
using Float=void (APIENTRY*)(GLint,GLfloat);
using Vec2=void (APIENTRY*)(GLint,GLfloat,GLfloat);
Object useProgram;
Float uniformFloat;
Vec2 uniformVec2;
GLint intensityLocation,timeLocation,texelLocation;
template<class T> T entry(const char* name)
{
#ifdef _WIN32
    auto address=wglGetProcAddress(name);
    const auto value=reinterpret_cast<std::uintptr_t>(address);
    if (value<=3 || value==~std::uintptr_t{0}) return nullptr;
#else
    auto address=glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name));
#endif
    return reinterpret_cast<T>(address);
}
bool initialize()
{
    auto create=entry<GLuint (APIENTRY*)(GLenum)>("glCreateShader");
    auto source=entry<void (APIENTRY*)(GLuint,GLsizei,const char* const*,const GLint*)>("glShaderSource");
    auto compile=entry<Object>("glCompileShader");
    auto status=entry<void (APIENTRY*)(GLuint,GLenum,GLint*)>("glGetShaderiv");
    auto make=entry<GLuint (APIENTRY*)()>("glCreateProgram");
    auto attach=entry<void (APIENTRY*)(GLuint,GLuint)>("glAttachShader");
    auto link=entry<Object>("glLinkProgram");
    auto linked=entry<void (APIENTRY*)(GLuint,GLenum,GLint*)>("glGetProgramiv");
    auto destroy=entry<Object>("glDeleteShader");
    auto locate=entry<GLint (APIENTRY*)(GLuint,const char*)>("glGetUniformLocation");
    auto integer=entry<void (APIENTRY*)(GLint,GLint)>("glUniform1i");
    useProgram=entry<Object>("glUseProgram");
    uniformFloat=entry<Float>("glUniform1f"); uniformVec2=entry<Vec2>("glUniform2f");
    if (!create||!source||!compile||!status||!make||!attach||!link||!linked||!destroy||!locate||!integer||!useProgram||!uniformFloat||!uniformVec2) return false;
    const char* fragment=R"(
#version 120
uniform sampler2D scene;
uniform float intensity,seconds;
uniform vec2 texel;
float noise(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
vec3 light(vec2 p) {
    vec3 c=texture2D(scene,clamp(p,vec2(0.0),vec2(1.0))).rgb;
    return c*smoothstep(0.55,0.98,max(c.r,max(c.g,c.b)));
}
void main() {
    vec2 uv=gl_TexCoord[0].xy;
    vec3 original=texture2D(scene,uv).rgb;
    float high=smoothstep(0.5,1.0,intensity);
    vec2 offset=(uv-0.5)*texel*high*1.4;
    vec3 color=vec3(texture2D(scene,uv+offset).r,original.g,texture2D(scene,uv-offset).b);
    vec2 radius=texel*mix(1.5,4.0,intensity);
    vec3 halo=(light(uv+vec2(radius.x,0.0))+light(uv-vec2(radius.x,0.0))+
        light(uv+vec2(0.0,radius.y))+light(uv-vec2(0.0,radius.y)))*0.25;
    color+=max(halo-original*0.75,vec3(0.0))*vec3(0.030,0.018,0.012)*intensity;
    float luminance=dot(original,vec3(0.2126,0.7152,0.0722));
    float grain=noise(floor(gl_FragCoord.xy)+floor(seconds*24.0)*vec2(17.0,53.0))-0.5;
    // No lifted black, no blur of the base image, and film-sized rather than UV-sized grain.
    color+=grain*0.018*intensity*sqrt(max(luminance,0.0))*(1.0-0.65*luminance);
    color+=vec3(0.008,-0.002,0.006)*high*luminance*(1.0-luminance);
    gl_FragColor=vec4(clamp(color,0.0,1.0),1.0);
}
)";
    const GLuint shader=create(0x8b30); source(shader,1,&fragment,nullptr); compile(shader);
    GLint ready=0; status(shader,0x8b81,&ready);
    if (ready) { program=make(); attach(program,shader); link(program); linked(program,0x8b82,&ready); }
    destroy(shader); if (!ready) return false;
    useProgram(program); integer(locate(program,"scene"),0);
    intensityLocation=locate(program,"intensity"); timeLocation=locate(program,"seconds"); texelLocation=locate(program,"texel");
    useProgram(0); glGenTextures(1,&frame); return frame!=0;
}
}
float extra::dramaAt(unsigned time)
{
    if (time<70000) return 0.4f*(1-ease(time,27000,35000));
    if (time<126000) return 0.8f*(1-ease(time,124000,126000));
    if (time<166000) return 0;
    if (time<211050) return 1-0.8f*ease(time,209500,211050);
    return 0.2f*(1-ease(time,219000,221200));
}
void extra::postProcess(int width,int height,unsigned time)
{
    const float base=dramaAt(time);
    if (!enabled || base<=0) return;
    if (!attempted) {
        attempted=true;
        if (!initialize()) { releasePostProcess(); attempted=true; std::fputs("ACT3: Film post effect unavailable\n",stderr); }
    }
    if (!frame) return;
    const float pulse=music::features().orchestralPulse;
    const float intensity=std::min(1.f,base*(0.82f+0.28f*pulse));
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TRANSFORM_BIT);
    glBindTexture(GL_TEXTURE_2D,frame);
    if (frameWidth!=width||frameHeight!=height) {
        glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,0,0,width,height,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
        frameWidth=width;frameHeight=height;
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
    glDisable(GL_BLEND); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); glDisable(GL_TEXTURE_GEN_R);
    glMatrixMode(GL_TEXTURE); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    useProgram(program); uniformFloat(intensityLocation,intensity); uniformFloat(timeLocation,time/1000.f);
    uniformVec2(texelLocation,1.f/width,1.f/height);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);
    glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
    useProgram(0);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_TEXTURE);glPopMatrix();glPopAttrib();
}
void extra::releasePostProcess()
{
    if (program) if(auto destroy=entry<Object>("glDeleteProgram")) destroy(program);
    if (frame) glDeleteTextures(1,&frame);
    program=frame=0;frameWidth=frameHeight=0;attempted=false;
}
