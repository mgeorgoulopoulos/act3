#include "color_grade.h"
#include "mode.h"
#include "shadow_cues.h"
#include "../compatibility/d3dx/d3dx9.h"
#include <cstdio>
#include <initializer_list>
#include <algorithm>

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
template<class T>
T glProc(const char* name)
{
#ifdef _WIN32
    auto entry = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::uintptr_t>(entry);
    if (value <= 3 || value == ~std::uintptr_t{0})
        return nullptr;
#else
    auto entry = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name));
#endif
    return reinterpret_cast<T>(entry);
}

GLuint program, frame, figure[2];
GLuint spaceFigure[3]{};
int spaceWidth[3]{},spaceHeight[3]{};
bool spaceRevealPass{};
int spaceRecovery{};
int figureWidth, figureHeight;
using UniformInt = void (APIENTRY*)(GLint, GLint);
UniformInt setUniform;
GLint figureModeLocation;
GLint spaceRecoveryLocation;
struct Lookup {
    const char* path;
    LPDIRECT3DTEXTURE9 texture{};
    bool attempted{};
};
Lookup eggplants{"data/textures/eggplants-grade.png"};
Lookup greetzBackground{"data/textures/greetz-background-grade.png"};
Lookup greetz{"data/textures/greetz-grade.png"};
Lookup shadow{"data/textures/shadow-grade.png"};
Lookup space{"data/textures/space-grade.png"};
Lookup credits{"data/textures/credits-grade.png"};
int frameWidth, frameHeight;
bool attempted;
using UseProgram = void (APIENTRY*)(GLuint);
using ActiveTexture = void (APIENTRY*)(GLenum);
UseProgram useProgram;
ActiveTexture activeTexture;
constexpr GLenum texture0 = 0x84c0, texture1 = 0x84c1;

const char* fragment = R"(
#version 120
uniform sampler2D scene;
uniform sampler2D grade;
uniform sampler2D figureColor, figureDepth;
uniform sampler2D backdropDepth;
uniform int figureMode;
uniform int spaceRecovery;
float plasmaSeed(vec2 uv) {
    vec3 rgb = texture2D(scene, clamp(uv, 0.0, 1.0)).rgb;
    float bright = max(rgb.r, max(rgb.g, rgb.b));
    float chroma = bright - min(rgb.r, min(rgb.g, rgb.b));
    return smoothstep(0.018, 0.11, chroma) * (1.0 - smoothstep(0.16, 0.29, bright));
}
vec3 deepenPlasma(vec2 uv, vec3 color) {
    // Grow and connect the dark particle cores using their existing shapes and motion.
    vec2 radius = vec2(0.007, 0.012);
    float nearby = plasmaSeed(uv + vec2(radius.x, 0.0))
                 + plasmaSeed(uv - vec2(radius.x, 0.0))
                 + plasmaSeed(uv + vec2(0.0, radius.y))
                 + plasmaSeed(uv - vec2(0.0, radius.y));
    float density = smoothstep(0.12, 0.72, 0.48 * plasmaSeed(uv) + 0.17 * nearby);
    return mix(color, color * vec3(0.18, 0.14, 0.28), density * 0.88);
}
vec3 slice(float r, vec2 gb) {
    return texture2D(grade, vec2((gb.y + 0.5) / 16.0,
                                 (r * 16.0 + gb.x + 0.5) / 256.0)).rgb;
}
void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec3 original = texture2D(scene, uv).rgb;
    if (figureMode == 3) {
        float figureZ = texture2D(figureDepth,uv).r;
        float backdropZ = texture2D(backdropDepth,uv).r;
        bool onPlaqueFigure = figureZ < backdropZ - 0.00000012;
        vec3 recovered = texture2D(figureColor,uv).rgb;
        gl_FragColor = vec4(mix(original,recovered,onPlaqueFigure ? float(spaceRecovery)/1000.0 : 0.0),1.0);
        return;
    }
    bool onFigure = figureMode != 0 && texture2D(figureDepth, uv).r < 0.999999;
    vec3 figure = vec3(0.0);
    float recovery = 0.0;
    if (onFigure) {
        figure = texture2D(figureColor, uv).rgb;
        if (figureMode == 2) {
            float shading = dot(figure, vec3(0.2126, 0.7152, 0.0722));
            figure = mix(vec3(0.26, 0.29, 0.48), vec3(0.76, 0.84, 1.0), shading);
            recovery = 0.55;
        } else {
            vec3 addedLight = max(original - figure, 0.0);
            recovery = 0.12 * smoothstep(0.10, 0.45, max(addedLight.r, addedLight.g));
        }
    }
    if (figureMode == 2 && !onFigure)
        original = deepenPlasma(uv, original);
    vec3 key = clamp(mix(original, figure, recovery), 0.0, 1.0) * 15.0;
    float r = floor(key.r);
    gl_FragColor = vec4(mix(slice(r, key.gb), slice(min(r + 1.0, 15.0), key.gb),
                            key.r - r), 1.0);
}
)";

bool initialize()
{
    auto createShader = glProc<GLuint (APIENTRY*)(GLenum)>("glCreateShader");
    auto source = glProc<void (APIENTRY*)(GLuint, GLsizei, const char* const*, const GLint*)>("glShaderSource");
    auto compile = glProc<UseProgram>("glCompileShader");
    auto createProgram = glProc<GLuint (APIENTRY*)()>("glCreateProgram");
    auto attach = glProc<void (APIENTRY*)(GLuint, GLuint)>("glAttachShader");
    auto link = glProc<UseProgram>("glLinkProgram");
    auto getShader = glProc<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetShaderiv");
    auto getProgram = glProc<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetProgramiv");
    auto deleteShader = glProc<UseProgram>("glDeleteShader");
    auto getUniform = glProc<GLint (APIENTRY*)(GLuint, const char*)>("glGetUniformLocation");
    setUniform = glProc<UniformInt>("glUniform1i");
    useProgram = glProc<UseProgram>("glUseProgram");
    activeTexture = glProc<ActiveTexture>("glActiveTexture");
    if (!createShader || !source || !compile || !createProgram || !attach || !link ||
        !getShader || !getProgram || !deleteShader || !getUniform || !setUniform ||
        !useProgram || !activeTexture)
        return false;

    const auto shader = createShader(0x8b30); // GL_FRAGMENT_SHADER
    source(shader, 1, &fragment, nullptr);
    compile(shader);
    GLint ready = 0;
    getShader(shader, 0x8b81, &ready); // GL_COMPILE_STATUS
    if (ready) {
        program = createProgram();
        attach(program, shader);
        link(program);
        getProgram(program, 0x8b82, &ready); // GL_LINK_STATUS
    }
    deleteShader(shader);
    if (!ready)
        return false;
    useProgram(program);
    setUniform(getUniform(program, "scene"), 0);
    setUniform(getUniform(program, "grade"), 1);
    setUniform(getUniform(program, "figureColor"), 2);
    setUniform(getUniform(program, "figureDepth"), 3);
    setUniform(getUniform(program, "backdropDepth"), 4);
    figureModeLocation = getUniform(program, "figureMode");
    spaceRecoveryLocation = getUniform(program, "spaceRecovery");
    useProgram(0);
    glGenTextures(1, &frame);
    return frame != 0;
}
}

void extra::captureShadowFigure()
{
    if (!enabled)
        return;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int width = viewport[2], height = viewport[3];
    if (!figure[0])
        glGenTextures(2, figure);
    glPushAttrib(GL_TEXTURE_BIT);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, figure[i]);
        if (width != figureWidth || height != figureHeight) {
            constexpr GLenum depth24 = 0x81a6;
            glCopyTexImage2D(GL_TEXTURE_2D, 0, i ? depth24 : GL_RGB8,
                viewport[0], viewport[1], width, height, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], width, height);
        }
    }
    glPopAttrib();
    figureWidth = width;
    figureHeight = height;
}

void extra::applyColorGrade(int width, int height, SceneGrade scene, unsigned time)
{
    if (!enabled)
        return;
    if (!attempted) {
        attempted = true;
        if (!initialize()) {
            releaseColorGrade();
            attempted = true;
            std::fputs("ACT3: Color grading unavailable\n", stderr);
        }
    }
    if (!frame)
        return;
    auto& lut = scene == SceneGrade::Credits ? credits : scene == SceneGrade::Space ? space : scene == SceneGrade::Shadow ? shadow :
        scene == SceneGrade::GreetzBackground ? greetzBackground :
        scene == SceneGrade::Greetz ? greetz : eggplants;
    if (!lut.attempted) {
        lut.attempted = true;
        d3d9::Device device;
        if (D3DXCreateTextureFromFileEx(&device, lut.path, 0, 0,
                D3DX_DEFAULT, 0, D3DFMT_DXT1, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                0, nullptr, nullptr, &lut.texture) < 0)
            std::fprintf(stderr, "ACT3: Could not load LUT %s\n", lut.path);
    }
    if (!lut.texture)
        return;
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    activeTexture(texture0);
    glBindTexture(GL_TEXTURE_2D, frame);
    if (frameWidth != width || frameHeight != height) {
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 0, 0, width, height, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        frameWidth = width;
        frameHeight = height;
    } else {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    }
    activeTexture(texture1);
    glBindTexture(GL_TEXTURE_2D, lut.texture->name);
    const bool useSpace = spaceRevealPass && spaceFigure[2] && spaceWidth[2]==width && spaceHeight[2]==height;
    const bool useFigure = scene == SceneGrade::Shadow && figure[1] &&
        figureWidth == width && figureHeight == height;
    if (useFigure) {
        for (int i = 0; i < 2; ++i) {
            activeTexture(texture0 + 2 + i);
            glBindTexture(GL_TEXTURE_2D, figure[i]);
        }
    }
    if (useSpace) {
        for (int i=0;i<2;++i) {
            activeTexture(texture0+2+i);
            glBindTexture(GL_TEXTURE_2D,spaceFigure[i+1]);
        }
        activeTexture(texture0+4); glBindTexture(GL_TEXTURE_2D,spaceFigure[0]);
    }
    activeTexture(texture0);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    useProgram(program);
    const int figureMode = useSpace ? 3 : useFigure ? (shadowNegativeActive(time) ? 2 : 1) : 0;
    setUniform(figureModeLocation, figureMode);
    setUniform(spaceRecoveryLocation,spaceRecovery);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
    useProgram(0);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glPopAttrib();
}

void extra::releaseColorGrade()
{
    if (program)
        if (auto destroy = glProc<UseProgram>("glDeleteProgram"))
            destroy(program);
    if (frame)
        glDeleteTextures(1, &frame);
    for (auto* lut : {&eggplants, &shadow, &greetzBackground, &greetz, &space, &credits}) {
        if (lut->texture)
            lut->texture->Release();
        lut->texture = nullptr;
        lut->attempted = false;
    }
    glDeleteTextures(2, figure);
    glDeleteTextures(3,spaceFigure);
    for(int i=0;i<3;++i) { spaceFigure[i]=0; spaceWidth[i]=spaceHeight[i]=0; }
    spaceRevealPass=false; spaceRecovery=0;
    figure[0] = figure[1] = 0;
    figureWidth = figureHeight = 0;
    program = frame = 0;
    attempted = false;
    frameWidth = frameHeight = 0;
}

namespace {
void captureSpaceImage(unsigned index)
{
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    const int width=viewport[2],height=viewport[3];
    if(!spaceFigure[index]) glGenTextures(1,&spaceFigure[index]);
    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D,spaceFigure[index]);
    if(spaceWidth[index]!=width || spaceHeight[index]!=height) {
        glCopyTexImage2D(GL_TEXTURE_2D,0,index==1?GL_RGB8:0x81a6,viewport[0],viewport[1],width,height,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
        spaceWidth[index]=width;spaceHeight[index]=height;
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,viewport[0],viewport[1],width,height);
    glPopAttrib();
}
}
void extra::captureSpaceBackdrop(unsigned time)
{
    if(enabled && time>=15000 && time<35000) captureSpaceImage(0);
}
void extra::captureSpaceFigures(unsigned time)
{
    if(enabled && time>=15000 && time<35000) { captureSpaceImage(1); captureSpaceImage(2); }
}
void extra::gradeSpaceReveal(unsigned time)
{
    if(!enabled || time<15000 || time>=35000) return;
    const float in=std::clamp((time-15000)/700.f,0.f,1.f);
    const float out=std::clamp((35000-time)/1000.f,0.f,1.f);
    spaceRecovery=static_cast<int>(460*in*out);
    GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
    spaceRevealPass=true;
    applyColorGrade(viewport[2],viewport[3],SceneGrade::Space);
    spaceRevealPass=false;
}

void extra::gradeGreetzBackground()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    applyColorGrade(viewport[2], viewport[3], SceneGrade::GreetzBackground);
}

void extra::gradeGreetzSurface()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    applyColorGrade(viewport[2], viewport[3], SceneGrade::Greetz);
}
