#include "d3dx9.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <new>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#else
#include <png.h>
#include <GL/glx.h>
#define APIENTRY
#endif
#include <GL/gl.h>

namespace {
bool readImage(const std::string& path, unsigned& width, unsigned& height,
    std::vector<unsigned char>& pixels)
{
#ifdef _WIN32
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    const std::wstring filename(path.begin(), path.end());
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateDecoderFromFilename(filename.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) ||
        FAILED(frame->GetSize(&width, &height)) || !width || !height ||
        width > 4096 || height > 4096 ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)))
        return false;
    pixels.resize(width * height * 4);
    return SUCCEEDED(converter->CopyPixels(nullptr, width * 4,
        static_cast<UINT>(pixels.size()), pixels.data()));
#else
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&image, path.c_str()))
        return false;
    width = image.width;
    height = image.height;
    image.format = PNG_FORMAT_RGBA;
    bool ok = width && height && width <= 4096 && height <= 4096;
    if (ok) {
        pixels.resize(PNG_IMAGE_SIZE(image));
        ok = png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr) != 0;
    }
    png_image_free(&image);
    return ok;
#endif
}
}

void d3d9::BaseTexture::Release()
{
    glDeleteTextures(1, &name);
    delete this;
}

d3d9::HRESULT D3DXCreateTextureFromFileEx(d3d9::Device* device, const char* file,
    unsigned width, unsigned height, unsigned levels, DWORD usage, d3d9::Format format,
    d3d9::Pool pool, DWORD filter, DWORD mipFilter, DWORD colorKey,
    void* info, void* palette, LPDIRECT3DTEXTURE9* result)
{
    using namespace d3d9;
    if (!result)
        return D3DERR_INVALIDCALL;
    *result = nullptr;
    if (!device || !file || width || height || levels != D3DX_DEFAULT || usage ||
        format != D3DFMT_DXT1 || pool != D3DPOOL_DEFAULT ||
        filter != D3DX_DEFAULT || mipFilter != D3DX_DEFAULT || colorKey || info || palette)
        return D3DERR_INVALIDCALL;

    std::string path(file);
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.compare(0, 14, "data/textures/") != 0)
        return D3DERR_INVALIDCALL;
    path = std::string(ACT3_TEXTURE_DIR) + '/' + path.substr(14);
    std::vector<unsigned char> pixels;
#ifdef _WIN32
    const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (com < 0 && com != static_cast<::HRESULT>(0x80010106L))
        return D3DERR_INVALIDCALL;
#endif
    const bool read = readImage(path, width, height, pixels);
#ifdef _WIN32
    if (com >= 0)
        CoUninitialize();
#endif
    if (!read)
        return D3DERR_INVALIDCALL;

    auto* texture = new (std::nothrow) BaseTexture{};
    if (!texture)
        return E_OUTOFMEMORY_RESULT;
    texture->target = GL_TEXTURE_2D;
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glGenTextures(1, &texture->name);
    if (!texture->name) {
        delete texture;
        return D3DERR_INVALIDCALL;
    }
    glBindTexture(GL_TEXTURE_2D, texture->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
    if (glGetError() != GL_NO_ERROR) {
        texture->Release();
        return D3DERR_INVALIDCALL;
    }
    *result = texture;
    return D3D_OK;
}

d3d9::HRESULT D3DXCreateCubeTextureFromFile(d3d9::Device* device, const char* file,
    LPDIRECT3DCUBETEXTURE9* result)
{
    using namespace d3d9;
    if (!result)
        return D3DERR_INVALIDCALL;
    *result = nullptr;
    if (!device || !file)
        return D3DERR_INVALIDCALL;
    std::string path(file);
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.compare(0, 14, "data/textures/") != 0)
        return D3DERR_INVALIDCALL;
    std::ifstream input(std::string(ACT3_TEXTURE_DIR) + '/' + path.substr(14),
        std::ios::binary);
    const std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(input), {});
    if (bytes.size() < 128)
        return D3DERR_INVALIDCALL;
    const auto word = [&](unsigned offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(bytes[offset + i]) << (i * 8);
        return value;
    };
    const auto size = word(16);
    const auto levels = (std::max)(1u, word(28));
    if (word(0) != 0x20534444 || word(4) != 124 || word(76) != 32 ||
        word(84) != D3DFMT_DXT1 || (word(112) & 0xfe00) != 0xfe00 ||
        !size || size > 4096 || size != word(12) || levels > 13 ||
        (size >> (levels - 1)) == 0)
        return D3DERR_INVALIDCALL;

    std::size_t faceBytes = 0;
    for (unsigned level = 0; level < levels; ++level) {
        const auto blocks = ((size >> level) + 3) / 4;
        faceBytes += blocks * blocks * 8;
    }
    if (faceBytes > (bytes.size() - 128) / 6)
        return D3DERR_INVALIDCALL;

    using Upload = void (APIENTRY*)(GLenum, GLint, GLenum, GLsizei, GLsizei,
        GLint, GLsizei, const void*);
#ifdef _WIN32
    auto upload = reinterpret_cast<Upload>(wglGetProcAddress("glCompressedTexImage2D"));
#else
    auto upload = reinterpret_cast<Upload>(glXGetProcAddressARB(
        reinterpret_cast<const GLubyte*>("glCompressedTexImage2D")));
#endif
    if (!upload)
        return D3DERR_INVALIDCALL;

    constexpr GLenum cubeMap = 0x8513, positiveX = 0x8515, rgbaDxt1 = 0x83f1;
    auto* texture = new (std::nothrow) BaseTexture{0, cubeMap};
    if (!texture)
        return E_OUTOFMEMORY_RESULT;
    GLint previous = 0;
    glGetIntegerv(0x8514, &previous); // GL_TEXTURE_BINDING_CUBE_MAP
    glGenTextures(1, &texture->name);
    glBindTexture(cubeMap, texture->name);
    glTexParameteri(cubeMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(cubeMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(cubeMap, GL_TEXTURE_WRAP_S, 0x812f); // GL_CLAMP_TO_EDGE
    glTexParameteri(cubeMap, GL_TEXTURE_WRAP_T, 0x812f);
    glTexParameteri(cubeMap, 0x8072, 0x812f); // GL_TEXTURE_WRAP_R
    for (unsigned face = 0; face < 6; ++face)
        upload(positiveX + face, 0, rgbaDxt1, size, size, 0,
            ((size + 3) / 4) * ((size + 3) / 4) * 8,
            bytes.data() + 128 + face * faceBytes);
    glBindTexture(cubeMap, static_cast<GLuint>(previous));
    if (!texture->name || glGetError() != GL_NO_ERROR) {
        texture->Release();
        return D3DERR_INVALIDCALL;
    }
    *result = texture;
    return D3D_OK;
}
