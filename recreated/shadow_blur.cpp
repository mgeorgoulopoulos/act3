#include "shadow_blur.h"
#include "../extra/shadow_focus.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

namespace {
GLuint frame = 0;
int allocatedSize = 0;

void drawTap(float u, float v)
{
    glBegin(GL_QUADS);
    glTexCoord2f(u, v); glVertex2f(-1, -1);
    glTexCoord2f(1 + u, v); glVertex2f(1, -1);
    glTexCoord2f(1 + u, 1 + v); glVertex2f(1, 1);
    glTexCoord2f(u, 1 + v); glVertex2f(-1, 1);
    glEnd();
}
}

void recreated::presentShadowBlur(int size, int width, int height, float offset, unsigned time)
{
    if (extra::postProcessShadow(size, width, height, time)) return;
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT |
        GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
    if (!frame)
        glGenTextures(1, &frame);
    glBindTexture(GL_TEXTURE_2D, frame);
    if (allocatedSize != size) {
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 0, 0, size, size, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        constexpr GLint clampToEdge = 0x812f;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
        allocatedSize = size;
    } else {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, size, size);
    }

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_BLEND);
    if (offset == 0) {
        glColor4f(1, 1, 1, 1);
        drawTap(0, 0);
    } else {
        glColor4f(0.25f, 0.25f, 0.25f, 1);
        drawTap(0, offset);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        drawTap(0, -offset);
        drawTap(offset, 0);
        drawTap(-offset, 0);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void recreated::releaseShadowBlur()
{
    extra::releaseShadowFocus();
    if (frame)
        glDeleteTextures(1, &frame);
    frame = 0;
    allocatedSize = 0;
}
