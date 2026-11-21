#include "space_engraving.h"
#include "mode.h"
#include <cstdint>
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
GLuint program;
bool attempted, active;
using Object = void (APIENTRY*)(GLuint);
Object useProgram;
template<class T>
T entry(const char* name)
{
#ifdef _WIN32
    auto address = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    if (value <= 3 || value == ~std::uintptr_t{0}) return nullptr;
#else
    auto address = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name));
#endif
    return reinterpret_cast<T>(address);
}
bool initialize()
{
    auto create = entry<GLuint (APIENTRY*)(GLenum)>("glCreateShader");
    auto source = entry<void (APIENTRY*)(GLuint, GLsizei, const char* const*, const GLint*)>("glShaderSource");
    auto compile = entry<Object>("glCompileShader");
    auto status = entry<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetShaderiv");
    auto makeProgram = entry<GLuint (APIENTRY*)()>("glCreateProgram");
    auto attach = entry<void (APIENTRY*)(GLuint, GLuint)>("glAttachShader");
    auto link = entry<Object>("glLinkProgram");
    auto programStatus = entry<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetProgramiv");
    auto destroy = entry<Object>("glDeleteShader");
    useProgram = entry<Object>("glUseProgram");
    if (!create || !source || !compile || !status || !makeProgram || !attach ||
        !link || !programStatus || !destroy || !useProgram) return false;
    const char* vertex = R"(
#version 120
varying vec3 normal;
void main() {
    normal = normalize(gl_NormalMatrix * gl_Normal);
    gl_Position = ftransform();
}
)";
    const char* fragment = R"(
#version 120
varying vec3 normal;
void main() {
    vec3 n = normalize(normal);
    float facing = abs(n.z);
    float edge = 1.0 - smoothstep(0.24, 0.35, facing);
    float crease = smoothstep(0.12, 0.30, length(dFdx(n)) + length(dFdy(n)));
    float ink = max(edge, crease * 0.75);
    if (ink < 0.08) discard;
    gl_FragColor = vec4(vec3(0.015), ink);
}
)";
    const GLuint shaders[] = {create(0x8b31),create(0x8b30)};
    const char* sources[] = {vertex,fragment};
    GLint ready = 1;
    program = makeProgram();
    for (int i=0;i<2;++i) {
        source(shaders[i],1,&sources[i],nullptr);
        compile(shaders[i]);
        GLint compiled;
        status(shaders[i],0x8b81,&compiled);
        ready &= compiled;
        attach(program,shaders[i]);
        destroy(shaders[i]);
    }
    link(program);
    GLint linked;
    programStatus(program,0x8b82,&linked);
    return ready && linked;
}
}
void extra::beginSpaceEngraving(unsigned time)
{
    if (!enabled || time >= 6500) return;
    if (!attempted) {
        attempted = true;
        if (!initialize()) {
            releaseSpaceEngraving();
            attempted = true;
            std::fputs("ACT3: Space engraving shader unavailable\n",stderr);
        }
    }
    if (!program) return;
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    useProgram(program);
    active = true;
}
void extra::endSpaceEngraving()
{
    if (!active) return;
    useProgram(0);
    glPopAttrib();
    active = false;
}
void extra::releaseSpaceEngraving()
{
    if (program)
        if (auto destroy = entry<Object>("glDeleteProgram")) destroy(program);
    program = 0;
    attempted = active = false;
}
