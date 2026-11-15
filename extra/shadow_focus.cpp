#include "shadow_focus.h"
#include "shadow_camera.h"
#include "mode.h"
#include <cstdint>
#include <cstdio>
#include <cmath>

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
T entry(const char* name)
{
#ifdef _WIN32
    auto p = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::uintptr_t>(p);
    if (value <= 3 || value == ~std::uintptr_t{0})
        return nullptr;
#else
    auto p = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name));
#endif
    return reinterpret_cast<T>(p);
}

using ObjectFn = void (APIENTRY*)(GLuint);
using UniformInt = void (APIENTRY*)(GLint, GLint);
using UniformFloat = void (APIENTRY*)(GLint, GLfloat);
using GetUniform = GLint (APIENTRY*)(GLuint, const char*);
using ActiveTexture = void (APIENTRY*)(GLenum);
GLuint program, images[2];
int cachedSize;
bool attempted;
ObjectFn useProgram, deleteProgram;
ActiveTexture activeTexture;
UniformFloat uniformFloat;
GLint focusLocation, strengthLocation, texelLocation;

const char* fragment = R"(
#version 120
uniform sampler2D image;
uniform sampler2D depth;
uniform float focus, strength, texel;
float distanceAt(vec2 uv) {
    return 1.0 / (1.0 - texture2D(depth, uv).r * 0.9999);
}
void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 center = texture2D(image, uv);
    float z = distanceAt(uv);
    float blur = smoothstep(0.09, 0.65, abs(focus / z - 1.0));
    vec3 total = center.rgb * 2.0;
    float weights = 2.0;
    for (int i = 0; i < 16; ++i) {
        float angle = float(i) * 2.399963;
        float ring = sqrt((float(i) + 0.5) / 16.0);
        vec2 tap = uv + vec2(cos(angle), sin(angle)) * (ring * blur * strength * texel);
        float sampleZ = distanceAt(tap);
        // A blurred backdrop must not pull color across the focused silhouette.
        float weight = smoothstep(0.65, 0.95, sampleZ / z);
        total += texture2D(image, tap).rgb * weight;
        weights += weight;
    }
    gl_FragColor = vec4(total / weights, center.a);
}
)";

bool initialize()
{
    auto createShader = entry<GLuint (APIENTRY*)(GLenum)>("glCreateShader");
    auto source = entry<void (APIENTRY*)(GLuint, GLsizei, const char* const*, const GLint*)>("glShaderSource");
    auto compile = entry<ObjectFn>("glCompileShader");
    auto createProgram = entry<GLuint (APIENTRY*)()>("glCreateProgram");
    auto attach = entry<void (APIENTRY*)(GLuint, GLuint)>("glAttachShader");
    auto link = entry<ObjectFn>("glLinkProgram");
    auto shaderStatus = entry<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetShaderiv");
    auto programStatus = entry<void (APIENTRY*)(GLuint, GLenum, GLint*)>("glGetProgramiv");
    auto deleteShader = entry<ObjectFn>("glDeleteShader");
    deleteProgram = entry<ObjectFn>("glDeleteProgram");
    auto locate = entry<GetUniform>("glGetUniformLocation");
    auto uniformInt = entry<UniformInt>("glUniform1i");
    uniformFloat = entry<UniformFloat>("glUniform1f");
    useProgram = entry<ObjectFn>("glUseProgram");
    activeTexture = entry<ActiveTexture>("glActiveTexture");
    if (!createShader || !source || !compile || !createProgram || !attach || !link ||
        !shaderStatus || !programStatus || !deleteShader || !deleteProgram ||
        !locate || !uniformInt || !uniformFloat || !useProgram || !activeTexture)
        return false;
    const auto shader = createShader(0x8b30); // GL_FRAGMENT_SHADER
    source(shader, 1, &fragment, nullptr);
    compile(shader);
    GLint success = 0;
    shaderStatus(shader, 0x8b81, &success); // GL_COMPILE_STATUS
    if (success) {
        program = createProgram();
        attach(program, shader);
        link(program);
        programStatus(program, 0x8b82, &success); // GL_LINK_STATUS
    }
    deleteShader(shader);
    if (!success)
        return false;
    useProgram(program);
    uniformInt(locate(program, "image"), 0);
    uniformInt(locate(program, "depth"), 1);
    focusLocation = locate(program, "focus");
    strengthLocation = locate(program, "strength");
    texelLocation = locate(program, "texel");
    useProgram(0);
    glGenTextures(2, images);
    return images[0] && images[1];
}
}

bool extra::postProcessShadow(int source, int width, int height, unsigned time)
{
    const float focus = shadowDollyDistance(time);
    if (!enabled || !focus)
        return false;
    if (!attempted) {
        attempted = true;
        if (!initialize()) {
            releaseShadowFocus();
            attempted = true;
            std::fputs("ACT3: Shadow depth of field unavailable\n", stderr);
        }
    }
    if (!images[0])
        return false;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT |
        GL_TRANSFORM_BIT | GL_CURRENT_BIT);
    for (int i = 0; i < 2; ++i) {
        activeTexture(0x84c0 + i); // GL_TEXTURE0 + i
        glBindTexture(GL_TEXTURE_2D, images[i]);
        if (cachedSize != source) {
            constexpr GLenum depth24 = 0x81a6;
            constexpr GLint clampToEdge = 0x812f;
            glCopyTexImage2D(GL_TEXTURE_2D, 0, i ? depth24 : GL_RGB8, 0, 0, source, source, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
        } else {
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, source, source);
        }
    }
    cachedSize = source;
    activeTexture(0x84c0);
    glViewport(0, 0, width, height);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    useProgram(program);
    uniformFloat(focusLocation, focus);
    uniformFloat(texelLocation, 1.f / source);
    const float phase = (time - 20000) / 7000.f;
    uniformFloat(strengthLocation, 9.f * std::sin(3.14159265f * phase));
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
    return true;
}

void extra::releaseShadowFocus()
{
    if (program)
        deleteProgram(program);
    glDeleteTextures(2, images);
    program = images[0] = images[1] = 0;
    cachedSize = 0;
    attempted = false;
}
