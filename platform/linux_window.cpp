#include <GL/glx.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include "../recreated/mesh_preview.h"
#include "../extra/mode.h"
#include "../extra/dramatometer.h"

namespace {
int reportError(const char* message)
{
    std::fprintf(stderr, "ACT3: %s\n", message);
    return 1;
}

int runWindow(Display* display)
{
    const int screen = DefaultScreen(display);
    int attributes[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, None};
    XVisualInfo* visual = glXChooseVisual(display, screen, attributes);
    if (!visual)
        return reportError("Could not find a suitable OpenGL visual.");

    const Window root = RootWindow(display, screen);
    XSetWindowAttributes settings{};
    settings.colormap = XCreateColormap(display, root, visual->visual, AllocNone);
    settings.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;
    Bool detectable=False;
    XkbSetDetectableAutoRepeat(display,True,&detectable);
    KeyCode physicalD=40;
    if(auto keyboard=XkbGetMap(display,XkbAllClientInfoMask,XkbUseCoreKbd)) {
        if(XkbGetNames(display,XkbKeyNamesMask,keyboard)==Success && keyboard->names && keyboard->names->keys)
            for(int key=keyboard->min_key_code;key<=keyboard->max_key_code;++key)
                if(std::memcmp(keyboard->names->keys[key].name,"AC03",4)==0) physicalD=static_cast<KeyCode>(key);
        XkbFreeKeyboard(keyboard,XkbAllComponentsMask,True);
    }
    int width = extra::enabled ? DisplayWidth(display, screen) * 2 / 3 : 800;
    int height = extra::enabled ? DisplayHeight(display, screen) * 2 / 3 : 600;
    const Window window = XCreateWindow(display, root, 0, 0, width, height, 0,
        visual->depth, InputOutput, visual->visual,
        CWColormap | CWEventMask, &settings);
    GLXContext context = glXCreateContext(display, visual, nullptr, True);
    XFree(visual);

    int result = 0;
    bool windowExists = true;
    if (context && glXMakeCurrent(display, window, context)) {
        XStoreName(display,window,preview::windowTitle());
        XMapWindow(display,window);XSync(display,False);
        preview::drawLoading(width,height);glXSwapBuffers(display,window);glFinish();
        if (!preview::load()) {
            preview::releaseTextures();
            glXMakeCurrent(display, None, nullptr); glXDestroyContext(display, context); XDestroyWindow(display, window); XFreeColormap(display, settings.colormap);
            return reportError("Could not initialize ACT3 scenes or soundtrack.");
        }
        const Atom protocols = XInternAtom(display, "WM_PROTOCOLS", False);
        Atom close = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &close, 1);
        XStoreName(display, window, preview::windowTitle());
        XMapWindow(display, window);
        glClearColor(0.05f, 0.09f, 0.18f, 1.0f);

        bool running = true;
        while (running) {
            if (!XPending(display)) {
                pollfd connection{ConnectionNumber(display), POLLIN, 0};
                if (poll(&connection, 1, 16) <= 0) {
                    preview::draw(width, height);
                    glXSwapBuffers(display, window);
                    if (preview::finished()) running = false;
                    continue;
                }
            }
            XEvent event{};
            XNextEvent(display, &event);
            switch (event.type) {
            case ConfigureNotify:
                width = event.xconfigure.width;
                height = event.xconfigure.height;
                // Fall through to repaint with the new size.
            case Expose:
                preview::draw(width, height);
                glXSwapBuffers(display, window);
                if (preview::finished()) running = false;
                break;
            case KeyPress:
                extra::dramatometerKey((event.xkey.state&ControlMask)!=0,
                    event.xkey.keycode==physicalD,true);
                running = XLookupKeysym(&event.xkey, 0) != XK_Escape;
                break;
            case KeyRelease: {
                const auto key=XLookupKeysym(&event.xkey,0);
                extra::dramatometerKey(key!=XK_Control_L && key!=XK_Control_R && (event.xkey.state&ControlMask)!=0,event.xkey.keycode==physicalD,false);
                break;
            }
            case ClientMessage:
                if (event.xclient.message_type == protocols &&
                    event.xclient.format == 32 &&
                    static_cast<Atom>(event.xclient.data.l[0]) == close)
                    running = false;
                break;
            case DestroyNotify:
                windowExists = false;
                running = false;
                break;
            }
        }
        preview::releaseTextures();
        glXMakeCurrent(display, None, nullptr);
    } else {
        result = reportError("Could not initialize OpenGL.");
    }

    if (context)
        glXDestroyContext(display, context);
    if (windowExists)
        XDestroyWindow(display, window);
    XFreeColormap(display, settings.colormap);
    return result;
}
}

int main(int argc, char** argv)
{
    if (argc == 1) preview::selectOption("--extra");
    for (int i = 1; i < argc; ++i)
        preview::selectOption(argv[i]);
    Display* display = XOpenDisplay(nullptr);
    if (!display)
        return reportError("Could not connect to the X display.");
    const int result = runWindow(display);
    XCloseDisplay(display);
    return result;
}
