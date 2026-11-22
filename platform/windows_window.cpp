#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include "../recreated/mesh_preview.h"
#include "../extra/mode.h"
#include "../extra/dramatometer.h"
#include <sstream>
#include "windows_launcher.h"
#include <vector>

namespace {
constexpr char windowClass[] = "Act3OpenGLWindow";
constexpr char windowTitle[] = "ACT3 | OpenGL";

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM key, LPARAM data)
{
    switch (message) {
    case WM_KILLFOCUS:
        extra::dramatometerKey(false,false,false);
        break;
    case WM_KEYDOWN:
        extra::dramatometerKey((GetKeyState(VK_CONTROL)&0x8000)!=0,((data>>16)&0xff)==0x20,true);
        if (key == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_KEYUP:
        extra::dramatometerKey((GetKeyState(VK_CONTROL)&0x8000)!=0,((data>>16)&0xff)==0x20,false);
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, key, data);
}

int reportError(const char* message)
{
    MessageBoxA(nullptr, message, windowTitle, MB_OK | MB_ICONERROR);
    return 1;
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR args, int showMode)
{
    SetProcessDPIAware();
    std::istringstream arguments(args);std::vector<std::string> tokens;
    for(std::string token;arguments>>token;)tokens.push_back(token);
    platform::LaunchOptions options;std::string error;
    if(!platform::parseOptions(tokens,options,error))return reportError(error.c_str());
    if(options.help){MessageBoxA(nullptr,"ACT3\n\n--extra / --original\n--resolution=1920x1080\n--fullscreen / --windowed\n--no-ui (skip launcher)\n\nEscape exits playback.","ACT3 / Options",MB_OK);return 0;}
    if(options.launcher && !platform::showLauncher(instance,options))return 0;
    POINT cursor{};GetCursorPos(&cursor);MONITORINFOEXA monitor{};monitor.cbSize=sizeof(monitor);
    GetMonitorInfoA(MonitorFromPoint(cursor,MONITOR_DEFAULTTOPRIMARY),&monitor);
    struct DisplayRestore { const char* device;bool changed=false;~DisplayRestore(){if(changed)ChangeDisplaySettingsExA(device,nullptr,nullptr,0,nullptr);} } restore{monitor.szDevice};
    if(options.fullscreen && options.width && (options.width!=monitor.rcMonitor.right-monitor.rcMonitor.left || options.height!=monitor.rcMonitor.bottom-monitor.rcMonitor.top)){
        DEVMODEA mode{};mode.dmSize=sizeof(mode);mode.dmPelsWidth=options.width;mode.dmPelsHeight=options.height;
        mode.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;
        if(ChangeDisplaySettingsExA(monitor.szDevice,&mode,nullptr,CDS_FULLSCREEN,nullptr)!=DISP_CHANGE_SUCCESSFUL)
            return reportError("That fullscreen resolution is unavailable. Choose a display mode from the resolution list.");
        restore.changed=true;
        GetMonitorInfoA(MonitorFromPoint(cursor,MONITOR_DEFAULTTOPRIMARY),&monitor);
    }
    WNDCLASSA windowType{};
    windowType.style = CS_OWNDC;
    windowType.lpfnWndProc = windowProc;
    windowType.hInstance = instance;
    windowType.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowType.lpszClassName = windowClass;
    if (!RegisterClassA(&windowType))
        return reportError("Could not register the window class.");

    const auto& bounds=monitor.rcMonitor;
    int width=options.width?options.width:bounds.right-bounds.left;
    int height=options.height?options.height:bounds.bottom-bounds.top;
    if(options.fullscreen){width=bounds.right-bounds.left;height=bounds.bottom-bounds.top;}
    DWORD style=options.fullscreen?WS_POPUP:WS_OVERLAPPEDWINDOW;
    RECT area{0,0,width,height};AdjustWindowRect(&area,style,FALSE);
    int x=options.fullscreen?bounds.left:bounds.left+((bounds.right-bounds.left)-(area.right-area.left))/2;
    int y=options.fullscreen?bounds.top:bounds.top+((bounds.bottom-bounds.top)-(area.bottom-area.top))/2;
    HWND window=CreateWindowA(windowClass,preview::windowTitle(),style,x,y,
        area.right-area.left,area.bottom-area.top,nullptr,nullptr,instance,nullptr);
    if (!window)
        return reportError("Could not create the window.");

    HDC dc = GetDC(window);
    PIXELFORMATDESCRIPTOR format{};
    format.nSize = sizeof(format);
    format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 24;
    format.cDepthBits = 24;
    const int pixelFormat = dc ? ChoosePixelFormat(dc, &format) : 0;
    HGLRC context = nullptr;
    if (pixelFormat && SetPixelFormat(dc, pixelFormat, &format))
        context = wglCreateContext(dc);

    int result = 0;
    if (context && wglMakeCurrent(dc, context)) {
        ShowWindow(window,showMode);
        RECT loading{};GetClientRect(window,&loading);
        preview::drawLoading(loading.right,loading.bottom);
        SwapBuffers(dc);glFinish();
        if (!preview::load()) {
            preview::releaseTextures();
            wglMakeCurrent(nullptr, nullptr); wglDeleteContext(context); ReleaseDC(window, dc); DestroyWindow(window);
            return reportError("Could not initialize ACT3 scenes or soundtrack.");
        }
        ShowWindow(window, showMode);
        SetTimer(window, 1, 16, nullptr);
        glClearColor(0.05f, 0.09f, 0.18f, 1.0f);

        MSG message{};
        BOOL received;
        while ((received = GetMessageA(&message, nullptr, 0, 0)) > 0) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
            RECT client{};
            GetClientRect(window, &client);
            preview::present(client.right, client.bottom);
            SwapBuffers(dc);
            if (preview::finished()) PostQuitMessage(0);
        }
        result = received == -1 ? 1 : static_cast<int>(message.wParam);
        KillTimer(window, 1);
        preview::releaseTextures();
        wglMakeCurrent(nullptr, nullptr);
    } else {
        result = reportError("Could not initialize OpenGL.");
    }

    if (context)
        wglDeleteContext(context);
    if (dc)
        ReleaseDC(window, dc);
    DestroyWindow(window);
    return result;
}
