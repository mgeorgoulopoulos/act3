#include "indexed_draw.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <GL/glx.h>
#define APIENTRY
#endif
#include <GL/gl.h>
#include <new>

namespace {
constexpr GLenum framebuffer = 0x8d40, framebufferBinding = 0x8ca6;
constexpr GLenum colorAttachment = 0x8ce0, depthAttachment = 0x8d00;
constexpr GLenum depth24 = 0x81a6, complete = 0x8cd5;

template<class T>
T entry(const char* name)
{
#ifdef _WIN32
    auto address = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    if (value <= 3 || value == ~std::uintptr_t{0})
        return nullptr;
#else
    auto address = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name));
#endif
    return reinterpret_cast<T>(address);
}

bool beginTarget(const d3d9::BaseTexture& target, GLint* previous, GLint* viewport)
{
    auto bind = entry<void (APIENTRY*)(GLenum, GLuint)>("glBindFramebuffer");
    if (!bind)
        return false;
    glGetIntegerv(framebufferBinding, previous);
    glGetIntegerv(GL_VIEWPORT, viewport);
    bind(framebuffer, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    return true;
}
}

void d3d9::BaseTexture::Release()
{
    if (framebuffer)
        if (auto destroy = entry<void (APIENTRY*)(GLsizei, const GLuint*)>("glDeleteFramebuffers"))
            destroy(1, &framebuffer);
    glDeleteTextures(1, &depth);
    glDeleteTextures(1, &name);
    delete this;
}

d3d9::HRESULT d3d9::Device::CreateTexture(unsigned width, unsigned height, unsigned levels,
    unsigned usage, Format format, Pool pool, BaseTexture** result, void* sharedHandle)
{
    if (!result)
        return D3DERR_INVALIDCALL;
    *result = nullptr;
    if (!width || !height || levels != 1 || usage != D3DUSAGE_RENDERTARGET ||
        format != D3DFMT_A8R8G8B8 || pool != D3DPOOL_DEFAULT || sharedHandle)
        return D3DERR_INVALIDCALL;
    auto gen = entry<void (APIENTRY*)(GLsizei, GLuint*)>("glGenFramebuffers");
    auto bind = entry<void (APIENTRY*)(GLenum, GLuint)>("glBindFramebuffer");
    auto attach = entry<void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint)>("glFramebufferTexture2D");
    auto status = entry<GLenum (APIENTRY*)(GLenum)>("glCheckFramebufferStatus");
    if (!gen || !bind || !attach || !status)
        return D3DERR_INVALIDCALL;

    auto* texture = new (std::nothrow) BaseTexture{};
    if (!texture)
        return E_OUTOFMEMORY_RESULT;
    texture->target = GL_TEXTURE_2D;
    texture->width = width;
    texture->height = height;
    glPushAttrib(GL_TEXTURE_BIT);
    glGenTextures(1, &texture->name);
    glBindTexture(GL_TEXTURE_2D, texture->name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &texture->depth);
    glBindTexture(GL_TEXTURE_2D, texture->depth);
    glTexImage2D(GL_TEXTURE_2D, 0, depth24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPopAttrib();

    GLint previous;
    glGetIntegerv(framebufferBinding, &previous);
    gen(1, &texture->framebuffer);
    bind(framebuffer, texture->framebuffer);
    attach(framebuffer, colorAttachment, GL_TEXTURE_2D, texture->name, 0);
    attach(framebuffer, depthAttachment, GL_TEXTURE_2D, texture->depth, 0);
    const bool ready = status(framebuffer) == complete;
    bind(framebuffer, previous);
    if (!ready) {
        texture->Release();
        return D3DERR_INVALIDCALL;
    }
    *result = texture;
    return D3D_OK;
}

d3d9::HRESULT d3d9::Device::BeginRenderTarget(BaseTexture* texture)
{
    if (renderTargetActive || !texture || !texture->framebuffer)
        return D3DERR_INVALIDCALL;
    SetTexture(0, nullptr);
    renderTargetActive = beginTarget(*texture, &previousFramebuffer, previousViewport);
    return renderTargetActive ? D3D_OK : D3DERR_INVALIDCALL;
}

d3d9::HRESULT d3d9::Device::EndRenderTarget()
{
    if (!renderTargetActive)
        return D3DERR_INVALIDCALL;
    auto bind = entry<void (APIENTRY*)(GLenum, GLuint)>("glBindFramebuffer");
    bind(framebuffer, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    renderTargetActive = false;
    return D3D_OK;
}
