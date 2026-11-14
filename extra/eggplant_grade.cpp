#include "eggplant_grade.h"
#include "mode.h"
#include "../compatibility/d3dx/d3dx9.h"
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
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

GLuint program, frame;
LPDIRECT3DTEXTURE9 lut;
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
vec3 slice(float r, vec2 gb) {
    return texture2D(grade, vec2((gb.y + 0.5) / 16.0,
                                 (r * 16.0 + gb.x + 0.5) / 256.0)).rgb;
}
void main() {
    vec3 key = clamp(texture2D(scene, gl_TexCoord[0].xy).rgb, 0.0, 1.0) * 15.0;
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
    auto setUniform = glProc<void (APIENTRY*)(GLint, GLint)>("glUniform1i");
    useProgram = glProc<UseProgram>("glUseProgram");
    activeTexture = glProc<ActiveTexture>("glActiveTexture");
    if (!createShader || !source || !compile || !createProgram || !attach || !link ||
        !getShader || !getProgram || !deleteShader || !getUniform || !setUniform ||
        !useProgram || !activeTexture)
        return false;

    d3d9::Device device;
    if (D3DXCreateTextureFromFileEx(&device, "data/textures/eggplants-grade.png", 0, 0,
            D3DX_DEFAULT, 0, D3DFMT_DXT1, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
            0, nullptr, nullptr, &lut) < 0)
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
    useProgram(0);
    glGenTextures(1, &frame);
    return frame != 0;
}
}

void extra::postProcessEggplants(int width, int height)
{
    if (!enabled)
        return;
    if (!attempted) {
        attempted = true;
        if (!initialize()) {
            releaseEggplantGrade();
            attempted = true;
            std::fputs("ACT3: Eggplant grading unavailable\n", stderr);
        }
    }
    if (!frame)
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
    glBindTexture(GL_TEXTURE_2D, lut->name);
    activeTexture(texture0);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    useProgram(program);
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
    glPopAttrib();
}

void extra::releaseEggplantGrade()
{
    if (program)
        if (auto destroy = glProc<UseProgram>("glDeleteProgram"))
            destroy(program);
    if (frame)
        glDeleteTextures(1, &frame);
    if (lut)
        lut->Release();
    program = frame = 0;
    lut = nullptr;
    attempted = false;
    frameWidth = frameHeight = 0;
}
