#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

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

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showMode)
{
    WNDCLASSA windowType{};
    windowType.style = CS_OWNDC;
    windowType.lpfnWndProc = windowProc;
    windowType.hInstance = instance;
    windowType.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowType.lpszClassName = windowClass;
    if (!RegisterClassA(&windowType))
        return reportError("Could not register the window class.");

    HWND window = CreateWindowA(windowClass, windowTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, instance, nullptr);
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
        ShowWindow(window, showMode);
        glClearColor(0.05f, 0.09f, 0.18f, 1.0f);

        MSG message{};
        BOOL received;
        while ((received = GetMessageA(&message, nullptr, 0, 0)) > 0) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
            glClear(GL_COLOR_BUFFER_BIT);
            SwapBuffers(dc);
        }
        result = received == -1 ? 1 : static_cast<int>(message.wParam);
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
