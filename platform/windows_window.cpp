#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include "../recreated/mesh_preview.h"
#include "../extra/mode.h"
#include <cstring>

namespace {
constexpr char windowClass[] = "Act3OpenGLWindow";
constexpr char windowTitle[] = "ACT3 | OpenGL";

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM key, LPARAM data)
{
    switch (message) {
    case WM_KEYDOWN:
        if (key == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
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
    extra::enabled = std::strcmp(args, "--extra") == 0;
    SetProcessDPIAware();
    WNDCLASSA windowType{};
    windowType.style = CS_OWNDC;
    windowType.lpfnWndProc = windowProc;
    windowType.hInstance = instance;
    windowType.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowType.lpszClassName = windowClass;
    if (!RegisterClassA(&windowType))
        return reportError("Could not register the window class.");

    RECT area{0, 0, 800, 600};
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (extra::enabled) {
        POINT cursor{};
        GetCursorPos(&cursor);
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (GetMonitorInfo(MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &monitor)) {
            const auto& bounds = monitor.rcMonitor;
            area.right = (bounds.right - bounds.left) * 2 / 3;
            area.bottom = (bounds.bottom - bounds.top) * 2 / 3;
            x = bounds.left + (bounds.right - bounds.left - area.right) / 2;
            y = bounds.top + (bounds.bottom - bounds.top - area.bottom) / 2;
        }
    }
    AdjustWindowRect(&area, WS_OVERLAPPEDWINDOW, FALSE);
    HWND window = CreateWindowA(windowClass, extra::enabled ? "ACT3 | Extra" : windowTitle,
        WS_OVERLAPPEDWINDOW, x, y, area.right - area.left, area.bottom - area.top,
        nullptr, nullptr, instance, nullptr);
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
        if (!preview::load()) {
            preview::releaseTextures();
            wglMakeCurrent(nullptr, nullptr); wglDeleteContext(context); ReleaseDC(window, dc); DestroyWindow(window);
            return reportError("Could not initialize EggplantScene.");
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
            preview::draw(client.right, client.bottom);
            SwapBuffers(dc);
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
