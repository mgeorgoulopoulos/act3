#include "indexed_draw.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifndef _WIN32
#include <GL/glx.h>
#define APIENTRY
#endif
#include <GL/gl.h>
#include <limits>
#include <cstring>

#include <new>

namespace d3d9 {
namespace {
class TextureUnit {
public:
    explicit TextureUnit(unsigned stage)
    {
        using Activate = void (APIENTRY*)(GLenum);
#ifdef _WIN32
        activate = reinterpret_cast<Activate>(wglGetProcAddress("glActiveTexture"));
#else
        activate = reinterpret_cast<Activate>(glXGetProcAddressARB(
            reinterpret_cast<const GLubyte*>("glActiveTexture")));
#endif
        if (activate) activate(0x84c0 + stage);
    }
    ~TextureUnit() { if (activate) activate(0x84c0); }
private:
    void (APIENTRY* activate)(GLenum){};
};
constexpr unsigned modelFormat = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
}

Device::~Device()
{
    SetStreamSource(0, nullptr, 0, 0);
    SetIndices(nullptr);
}

HRESULT Device::CreateVertexBuffer(unsigned length, unsigned usage, unsigned format,
    Pool pool, VertexBuffer** buffer, void** sharedHandle)
{
    if (!buffer)
        return D3DERR_INVALIDCALL;
    *buffer = nullptr;
    if (!length || (usage & ~D3DUSAGE_WRITEONLY) || pool != D3DPOOL_SYSTEMMEM ||
        sharedHandle || (format != D3DFVF_XYZ && format != modelFormat &&
            format != (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)))
        return D3DERR_INVALIDCALL;
    try {
        *buffer = new VertexBuffer(length);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY_RESULT;
    }
    return D3D_OK;
}

HRESULT Device::CreateIndexBuffer(unsigned length, unsigned usage, Format format,
    Pool pool, IndexBuffer** buffer, void** sharedHandle)
{
    if (!buffer)
        return D3DERR_INVALIDCALL;
    *buffer = nullptr;
    if (!length || length % sizeof(std::uint16_t) || (usage & ~D3DUSAGE_WRITEONLY) ||
        format != D3DFMT_INDEX16 || pool != D3DPOOL_SYSTEMMEM || sharedHandle)
        return D3DERR_INVALIDCALL;
    try {
        *buffer = new IndexBuffer(length);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY_RESULT;
    }
    return D3D_OK;
}

HRESULT Device::SetStreamSource(unsigned stream, VertexBuffer* buffer, unsigned offset,
    unsigned vertexStride)
{
    if (stream || (buffer && (offset >= buffer->length || offset % sizeof(float) ||
                             !vertexStride || vertexStride % sizeof(float))))
        return D3DERR_INVALIDCALL;
    if (buffer)
        buffer->AddRef();
    if (vertexBuffer)
        vertexBuffer->Release();
    vertexBuffer = buffer;
    streamOffset = offset;
    stride = vertexStride;
    return D3D_OK;
}

HRESULT Device::SetIndices(IndexBuffer* buffer)
{
    if (buffer)
        buffer->AddRef();
    if (indexBuffer)
        indexBuffer->Release();
    indexBuffer = buffer;
    return D3D_OK;
}

HRESULT Device::SetMaterial(const Material* material)
{
    if (!material)
        return D3DERR_INVALIDCALL;
    const auto apply = [](GLenum property, const Color& color) {
        const GLfloat rgba[] = {color.r, color.g, color.b, color.a};
        glMaterialfv(GL_FRONT_AND_BACK, property, rgba);
    };
    apply(GL_AMBIENT, material->Ambient);
    apply(GL_DIFFUSE, material->Diffuse);
    apply(GL_SPECULAR, material->Specular);
    apply(GL_EMISSION, material->Emissive);
    const float power = material->Power;
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, power > 0 ? (power < 128 ? power : 128) : 0);
    return D3D_OK;
}

HRESULT Device::SetTexture(unsigned stage, BaseTexture* texture)
{
    if (stage > 1)
        return stage < 4 && !texture ? D3D_OK : D3DERR_INVALIDCALL;
    TextureUnit unit(stage);
    constexpr GLenum cubeMap = 0x8513;
    glDisable(GL_TEXTURE_2D);
    glDisable(cubeMap);
    GLint matrixMode;
    glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    if (texture && texture->framebuffer) {
        glTranslatef(0, 1, 0);
        glScalef(1, -1, 1);
    }
    glMatrixMode(matrixMode);
    if (texture) {
        glBindTexture(texture->target, texture->name);
        glEnable(texture->target);
    }
    return D3D_OK;
}

HRESULT Device::SetTextureStageState(unsigned stage, TextureStageState state, unsigned value)
{
    if (stage > 1) return D3DERR_INVALIDCALL;
    TextureUnit unit(stage);
    if (stage == 1 && state == D3DTSS_COLOROP && value == 11) {
        // D3DTOP_ADDSMOOTH = previous + texture * (1 - previous).
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, 0x8570);
        glTexEnvi(GL_TEXTURE_ENV, 0x8571, 0x8575);
        glTexEnvi(GL_TEXTURE_ENV, 0x8580, 0x8576);
        glTexEnvi(GL_TEXTURE_ENV, 0x8581, 0x8578);
        glTexEnvi(GL_TEXTURE_ENV, 0x8582, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, 0x8590, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, 0x8591, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, 0x8592, GL_SRC_COLOR);
        const GLfloat white[] = {1, 1, 1, 1};
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, white);
        return D3D_OK;
    }
    if (stage == 0 && (state == D3DTSS_ALPHAOP || state == D3DTSS_ALPHAARG1 || state == D3DTSS_ALPHAARG2))
        return D3D_OK;
    if (stage == 0 && state == D3DTSS_COLOROP && value == D3DTOP_MODULATE) {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        return D3D_OK;
    }
    if (stage == 0 && ((state == D3DTSS_COLORARG1 && value == D3DTA_DIFFUSE) ||
                      (state == D3DTSS_COLORARG2 && value == D3DTA_TEXTURE)))
        return D3D_OK;
    const auto generation = value & 0xffff0000u;
    if (state != D3DTSS_TEXCOORDINDEX ||
        (value & 0xffffu) > 1 ||
        (generation != D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR &&
         value != D3DTSS_TCI_PASSTHRU))
        return D3DERR_INVALIDCALL;

    constexpr GLint reflectionMap = 0x8512; // GL_REFLECTION_MAP
    if (generation == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) {
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, reflectionMap);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, reflectionMap);
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, reflectionMap);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_GEN_R);
    } else {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
    }
    return D3D_OK;
}

HRESULT Device::SetRenderState(RenderState state, unsigned value)
{
    switch (state) {
    case D3DRS_FILLMODE:
        if (value != D3DFILL_SOLID)
            return D3DERR_INVALIDCALL;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case D3DRS_ZENABLE:
    case D3DRS_ALPHABLENDENABLE:
    case D3DRS_LIGHTING: {
        const GLenum capability = state == D3DRS_ZENABLE ? GL_DEPTH_TEST :
            state == D3DRS_LIGHTING ? GL_LIGHTING : GL_BLEND;
        if (value)
            glEnable(capability);
        else
            glDisable(capability);
        break;
    }
    case D3DRS_ALPHATESTENABLE:
        if (value) glEnable(GL_ALPHA_TEST); else glDisable(GL_ALPHA_TEST);
        break;
    case D3DRS_NORMALIZENORMALS:
        if (value) glEnable(GL_NORMALIZE); else glDisable(GL_NORMALIZE);
        break;
    case D3DRS_ZWRITEENABLE: glDepthMask(value ? GL_TRUE : GL_FALSE); break;
    case D3DRS_LOCALVIEWER: glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, value != 0); break;
    case D3DRS_SPECULARENABLE:
        glLightModeli(0x81f8, value ? 0x81fa : 0x81f9);
        break;
    case D3DRS_AMBIENT: {
        const GLfloat rgba[] = {((value >> 16) & 255) / 255.0f,
            ((value >> 8) & 255) / 255.0f, (value & 255) / 255.0f,
            ((value >> 24) & 255) / 255.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, rgba);
        break;
    }
    case D3DRS_CULLMODE:
        if (value == D3DCULL_NONE) {
            glDisable(GL_CULL_FACE);
            break;
        }
        if (value != D3DCULL_CW && value != D3DCULL_CCW)
            return D3DERR_INVALIDCALL;
        glFrontFace(value == D3DCULL_CW ? GL_CCW : GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        break;
    case D3DRS_SRCBLEND:
    case D3DRS_DESTBLEND:
        if (value != 5 && value != 6 && value != D3DBLEND_ZERO && value != D3DBLEND_ONE &&
            !(state == D3DRS_SRCBLEND && value == D3DBLEND_SRCCOLOR) &&
            !(state == D3DRS_DESTBLEND && (value == D3DBLEND_INVDESTCOLOR || value == D3DBLEND_INVSRCCOLOR)))
            return D3DERR_INVALIDCALL;
        (state == D3DRS_SRCBLEND ? sourceBlend : destinationBlend) = value;
        glBlendFunc(sourceBlend == 5 ? GL_SRC_ALPHA : sourceBlend == 6 ? GL_ONE_MINUS_SRC_ALPHA : sourceBlend == D3DBLEND_SRCCOLOR ? GL_SRC_COLOR : sourceBlend == D3DBLEND_ONE ? GL_ONE : GL_ZERO,
            destinationBlend == 5 ? GL_SRC_ALPHA : destinationBlend == 6 ? GL_ONE_MINUS_SRC_ALPHA :
            destinationBlend == D3DBLEND_INVDESTCOLOR ? GL_ONE_MINUS_DST_COLOR :
                destinationBlend == D3DBLEND_INVSRCCOLOR ? GL_ONE_MINUS_SRC_COLOR :
                destinationBlend == D3DBLEND_ONE ? GL_ONE : GL_ZERO);
        break;
    default: return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT Device::SetTransform(TransformState state, const Matrix* matrix)
{
    if (!matrix)
        return D3DERR_INVALIDCALL;
    switch (state) {
    case D3DTS_WORLD: world = *matrix; break;
    case D3DTS_VIEW: view = *matrix; break;
    case D3DTS_PROJECTION:
        projection = *matrix;
        if (!renderTargetActive)
            projection._11 *= projectionHorizontalScale;
        break;
    default: return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT Device::SetLight(unsigned index, const Light* light)
{
    if (index >= 2 || !light || light->Type != D3DLIGHT_DIRECTIONAL)
        return D3DERR_INVALIDCALL;
    lights[index] = *light;
    return D3D_OK;
}

HRESULT Device::LightEnable(unsigned index, int enable)
{
    if (index >= 2 || (enable && lights[index].Type != D3DLIGHT_DIRECTIONAL))
        return D3DERR_INVALIDCALL;
    lightEnabled[index] = enable != 0;
    return D3D_OK;
}

HRESULT Device::GetTransform(TransformState state, Matrix* matrix) const
{
    if (!matrix)
        return D3DERR_INVALIDCALL;
    switch (state) {
    case D3DTS_WORLD: *matrix = world; break;
    case D3DTS_VIEW: *matrix = view; break;
    case D3DTS_PROJECTION: *matrix = projection; break;
    default: return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

void Device::applyTransforms() const
{
    // Convert D3D clip depth [0,w] to OpenGL clip depth [-w,w].
    Matrix clip;
    clip.m[2][2] = 2;
    clip.m[3][2] = -1;
    const auto glProjection = projection * clip;
    const auto modelView = world * view;
    // OpenGL reads the same memory as column-major, transposing both matrices.
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&glProjection.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&view.m[0][0]);
    for (unsigned i = 0; i < 2; ++i) {
        const GLenum slot = GL_LIGHT0 + i;
        if (!lightEnabled[i]) {
            glDisable(slot);
            continue;
        }
        const auto& light = lights[i];
        // D3D stores the direction of travel. OpenGL takes the direction to the light.
        const GLfloat direction[] = {-light.Direction.x, -light.Direction.y,
            -light.Direction.z, 0};
        glLightfv(slot, GL_POSITION, direction);
        const auto apply = [slot](GLenum property, const Color& color) {
            const GLfloat rgba[] = {color.r, color.g, color.b, color.a};
            glLightfv(slot, property, rgba);
        };
        apply(GL_AMBIENT, light.Ambient);
        apply(GL_DIFFUSE, light.Diffuse);
        apply(GL_SPECULAR, light.Specular);
        glEnable(slot);
    }
    glLoadMatrixf(&modelView.m[0][0]);
}

HRESULT Device::Clear(unsigned count, const void* rects, unsigned flags, unsigned color,
    float depth, unsigned stencil)
{
    if (count || rects || (flags & ~3u) || stencil)
        return D3DERR_INVALIDCALL;
    glClearColor(((color >> 16) & 255)/255.f, ((color >> 8) & 255)/255.f,
        (color & 255)/255.f, ((color >> 24) & 255)/255.f);
    glClearDepth(depth);
    glClear((flags & 1 ? GL_COLOR_BUFFER_BIT : 0) | (flags & 2 ? GL_DEPTH_BUFFER_BIT : 0));
    return D3D_OK;
}

HRESULT Device::SetSamplerState(unsigned stage, unsigned state, unsigned value)
{
    // 2D and cube textures are already uploaded with linear base-level filtering.
    return stage == 0 && state >= 5 && state <= 7 && value == 2 ? D3D_OK : D3DERR_INVALIDCALL;
}

HRESULT Device::DrawPrimitive(PrimitiveType type, unsigned start, unsigned count) const
{
    const bool model = fvf == modelFormat;
    if (type != D3DPT_TRIANGLELIST ||
        (!model && fvf != (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)) ||
        !vertexBuffer || vertexBuffer->locked || stride != (model ? 32u : 24u))
        return D3DERR_INVALIDCALL;
    const auto available = (vertexBuffer->length - streamOffset) / stride;
    if (start > available || count > (available - start) / 3)
        return D3DERR_INVALIDCALL;

    applyTransforms();
    const auto* bytes = reinterpret_cast<const unsigned char*>(vertexBuffer->storage.data());
    if (model) {
        const auto* vertex = vertexBuffer->storage.data() + streamOffset / sizeof(float);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(3, GL_FLOAT, stride, vertex);
        glNormalPointer(GL_FLOAT, stride, vertex + 3);
        glTexCoordPointer(2, GL_FLOAT, stride, vertex + 6);
        glDrawArrays(GL_TRIANGLES, start, count * 3);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        return D3D_OK;
    }
    glBegin(GL_TRIANGLES);
    for (unsigned i = 0; i < count * 3; ++i) {
        const auto* vertex = bytes + streamOffset + (start + i) * stride;
        std::uint32_t color;
        std::memcpy(&color, vertex + 12, 4);
        float xyz[3], uv[2];
        std::memcpy(xyz, vertex, sizeof(xyz));
        std::memcpy(uv, vertex + 16, sizeof(uv));
        glColor4ub((color >> 16) & 255, (color >> 8) & 255, color & 255, color >> 24);
        glTexCoord2fv(uv);
        glVertex3fv(xyz);
    }
    glEnd();
    return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitive(PrimitiveType type, int baseVertex,
    unsigned minVertex, unsigned vertexCount, unsigned startIndex,
    unsigned primitiveCount) const
{
    const auto glLimit = (std::numeric_limits<GLsizei>::max)();
    const auto* vertices = vertexBuffer && !vertexBuffer->locked ? vertexBuffer->storage.data() : nullptr;
    const auto* indices = indexBuffer && !indexBuffer->locked ? indexBuffer->storage.data() : nullptr;
    const auto indexCount = indexBuffer ? indexBuffer->storage.size() : 0;
    const bool model = fvf == modelFormat;
    const bool grid = fvf == (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    const auto minimumStride = (model ? 8 : grid ? 6 : 3) * sizeof(float);
    if (type != D3DPT_TRIANGLELIST || (!model && !grid && fvf != D3DFVF_XYZ) ||
        !vertices || !indices || baseVertex < 0 ||
        stride < minimumStride || stride % sizeof(float) != 0 ||
        stride > static_cast<std::size_t>(glLimit) ||
        startIndex > indexCount || primitiveCount > (indexCount - startIndex) / 3 ||
        primitiveCount > static_cast<unsigned>(glLimit / 3))
        return D3DERR_INVALIDCALL;

    const auto streamCount = (vertexBuffer->length - streamOffset) / stride;
    const auto base = static_cast<std::size_t>(baseVertex);
    if (base > streamCount || minVertex > streamCount - base ||
        vertexCount > streamCount - base - minVertex)
        return D3DERR_INVALIDCALL;

    const auto count = static_cast<GLsizei>(primitiveCount * 3);
    const auto* firstIndex = indices + startIndex;
    for (GLsizei i = 0; i < count; ++i) {
        const auto index = firstIndex[i];
        if (index < minVertex || index - minVertex >= vertexCount)
            return D3DERR_INVALIDCALL;
    }

    const auto* firstVertex = vertices + (streamOffset + base * stride) / sizeof(float);
    applyTransforms();
    if (grid) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(firstVertex);
        glBegin(GL_TRIANGLES);
        for (GLsizei i = 0; i < count; ++i) {
            const auto* vertex = bytes + firstIndex[i] * stride;
            float xyz[3], uv[2];
            std::uint32_t color;
            std::memcpy(xyz, vertex, sizeof(xyz));
            std::memcpy(&color, vertex + 12, sizeof(color));
            std::memcpy(uv, vertex + 16, sizeof(uv));
            glColor4ub((color >> 16) & 255, (color >> 8) & 255, color & 255, color >> 24);
            glTexCoord2fv(uv);
            glVertex3fv(xyz);
        }
        glEnd();
        return D3D_OK;
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex);
    if (model) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, static_cast<GLsizei>(stride), firstVertex + 3);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, static_cast<GLsizei>(stride), firstVertex + 6);
    }
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, firstIndex);
    if (model) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    return D3D_OK;
}
}
