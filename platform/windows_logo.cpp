#include "windows_logo.h"
#include <wincodec.h>
#include <algorithm>
HBITMAP platform::loadLauncherFigure(HINSTANCE instance,int height){
    auto resource=FindResourceW(instance,MAKEINTRESOURCEW(201),MAKEINTRESOURCEW(10));
    if(!resource)return nullptr;
    auto data=LockResource(LoadResource(instance,resource));auto size=SizeofResource(instance,resource);
    const HRESULT apartment=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory=nullptr;IWICStream* stream=nullptr;IWICBitmapDecoder* decoder=nullptr;
    IWICBitmapFrameDecode* frame=nullptr;IWICBitmapScaler* scaler=nullptr;IWICFormatConverter* converter=nullptr;
    HBITMAP bitmap=nullptr;
    auto load=[&](){
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) ||
            FAILED(factory->CreateStream(&stream)) ||
            FAILED(stream->InitializeFromMemory(static_cast<BYTE*>(data),size)) ||
            FAILED(factory->CreateDecoderFromStream(stream,nullptr,WICDecodeMetadataCacheOnLoad,&decoder)) ||
            FAILED(decoder->GetFrame(0,&frame)))return;
        UINT w=0,h=0;frame->GetSize(&w,&h);if(!w||!h)return;
        int width=std::max(1,int(double(w)*height/h));
        if(FAILED(factory->CreateBitmapScaler(&scaler)) ||
            FAILED(scaler->Initialize(frame,width,height,WICBitmapInterpolationModeFant)) ||
            FAILED(factory->CreateFormatConverter(&converter)) ||
            FAILED(converter->Initialize(scaler,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return;
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
        info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr;bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        if(bitmap && FAILED(converter->CopyPixels(nullptr,width*4,width*height*4,static_cast<BYTE*>(pixels)))){DeleteObject(bitmap);bitmap=nullptr;}
    };load();
    if(converter)converter->Release();if(scaler)scaler->Release();if(frame)frame->Release();
    if(decoder)decoder->Release();if(stream)stream->Release();if(factory)factory->Release();
    if(SUCCEEDED(apartment))CoUninitialize();return bitmap;
}
