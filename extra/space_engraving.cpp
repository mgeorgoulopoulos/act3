#include "space_engraving.h"
#include "mode.h"
#include "space_camera.h"
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
using UniformFloat = void (APIENTRY*)(GLint, GLfloat);
UniformFloat uniformFloat;
GLint revealLocation;
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
    auto locate = entry<GLint (APIENTRY*)(GLuint, const char*)>("glGetUniformLocation");
    auto uniformInt = entry<void (APIENTRY*)(GLint, GLint)>("glUniform1i");
    uniformFloat = entry<UniformFloat>("glUniform1f");
    if (!locate || !uniformInt || !uniformFloat) return false;
    const char* vertex = R"(
#version 120
varying vec3 normal;
varying vec3 eyePosition;
void main() {
    eyePosition = (gl_ModelViewMatrix * gl_Vertex).xyz;
    normal = normalize(gl_NormalMatrix * gl_Normal);
    gl_Position = ftransform();
}
)";
    const char* fragment = R"(
#version 120
varying vec3 normal;
varying vec3 eyePosition;
uniform float reveal;
uniform samplerCube environment;
void main() {
    vec3 n = normalize(normal);
    float facing = abs(n.z);
    float edge = 1.0 - smoothstep(0.24, 0.35, facing);
    float crease = smoothstep(0.12, 0.30, length(dFdx(n)) + length(dFdy(n)));
    float ink = max(edge, crease * 0.75);
    float alpha = mix(ink,1.0,reveal);
    if (alpha < 0.08) discard;
    vec3 base = clamp(gl_FrontMaterial.ambient.rgb * gl_LightModel.ambient.rgb
        + gl_FrontMaterial.diffuse.rgb * max(0.0,dot(n,normalize(gl_LightSource[0].position.xyz))),0.0,1.0);
    vec3 reflection = textureCube(environment,reflect(normalize(eyePosition),n)).rgb;
    vec3 shaded = base + reflection*(1.0-base);
    gl_FragColor = vec4(mix(vec3(0.015),shaded,reveal/max(alpha,0.001)),alpha);
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
    if (ready && linked) {
        useProgram(program);
        revealLocation = locate(program,"reveal");
        uniformInt(locate(program,"environment"),1);
        useProgram(0);
    }
    return ready && linked;
}
}
void extra::beginSpaceEngraving(unsigned time)
{
    if (!enabled) return;
    const unsigned sceneTime = time;
    time = spaceCameraTime(time);
    if (time >= 11000) return;
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
    // Same reveal interval as the first recorded turn: 20329..22338 ms.
    float phase = (float(sceneTime)-20329)/2009.f;
    phase = phase < 0 ? 0 : phase > 1 ? 1 : phase;
    uniformFloat(revealLocation,phase*phase*(3-2*phase));
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
