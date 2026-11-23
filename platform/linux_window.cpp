#include "recreated/assets.h"
#include <cstdio>
#include <GL/glx.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include "launch_options.h"
#include "linux_display.h"
#include "linux_launcher.h"
#include <algorithm>
#include "../recreated/mesh_preview.h"
#include "../extra/mode.h"
#include "../extra/dramatometer.h"

namespace {
int reportError(const char* message)
{
    std::fprintf(stderr, "ACT3: %s\n", message);
    return 1;
}

int runWindow(Display* display,const platform::LaunchOptions& options)
{
    platform::LinuxDisplay monitor(display);std::string displayError;
    if(!monitor.configure(options,displayError))return reportError(displayError.c_str());
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
    int width = options.width ? options.width : monitor.width;
    int height = options.height ? options.height : monitor.height;
    std::printf("  %d x %d / %s / Escape to exit\n\n",width,height,options.fullscreen?"fullscreen":"windowed");
    const Window window = XCreateWindow(display, root, monitor.x, monitor.y, width, height, 0,
        visual->depth, InputOutput, visual->visual,
        CWColormap | CWEventMask, &settings);
    if(options.fullscreen){
        Atom state=XInternAtom(display,"_NET_WM_STATE",False);
        Atom full=XInternAtom(display,"_NET_WM_STATE_FULLSCREEN",False);
        XChangeProperty(display,window,state,XA_ATOM,32,PropModeReplace,reinterpret_cast<unsigned char*>(&full),1);
    }
    GLXContext context = glXCreateContext(display, visual, nullptr, True);
    XFree(visual);

    int result = 0;
    bool windowExists = true;
    if (context && glXMakeCurrent(display, window, context)) {
        XStoreName(display,window,preview::windowTitle());
        XMapWindow(display,window);XSync(display,False);
        preview::drawLoading(width,height);glXSwapBuffers(display,window);glFinish();
        if (!preview::load(!options.checkScenes)) {
            preview::releaseTextures();
            glXMakeCurrent(display, None, nullptr); glXDestroyContext(display, context); XDestroyWindow(display, window); XFreeColormap(display, settings.colormap);
            return reportError("Could not initialize ACT3 scenes or soundtrack.");
        }
        if(options.checkScenes){
            preview::releaseTextures();glXMakeCurrent(display,None,nullptr);glXDestroyContext(display,context);
            XDestroyWindow(display,window);XFreeColormap(display,settings.colormap);std::puts("ACT3 scenes OK");return 0;
        }
        std::puts("  Ready. Enjoy ACT3.\n");
        const Atom protocols = XInternAtom(display, "WM_PROTOCOLS", False);
        Atom close = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &close, 1);
        XStoreName(display, window, preview::windowTitle());
        XMapWindow(display, window);
        glClearColor(0.05f, 0.09f, 0.18f, 1.0f);

        bool running = true;
        while (running) {
            // Socket readiness need not mean an application event is queued.
            // Drain a bounded batch, then render even during continuous X11 traffic.
            const int queued = XPending(display);
            for (int remaining = queued < 64 ? queued : 64; remaining && running; --remaining) {
                XEvent event{};
                XNextEvent(display, &event);
                switch (event.type) {
                case ConfigureNotify:
                    width = event.xconfigure.width;
                    height = event.xconfigure.height;
                    break;
                case Expose:
                    break;
                case KeyPress:
                    extra::dramatometerKey((event.xkey.state&ControlMask)!=0,
                        event.xkey.keycode==physicalD,true);
                    if (XLookupKeysym(&event.xkey, 0) == XK_Escape) running = false;
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
            if (!running) break;
            preview::present(width, height);
            glXSwapBuffers(display, window);
            if (preview::finished()) break;
            if (!XPending(display)) {
                pollfd connection{ConnectionNumber(display), POLLIN, 0};
                poll(&connection, 1, 16);
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
    platform::LaunchOptions options;std::string error;std::vector<std::string> arguments(argv+1,argv+argc);
    const bool color=isatty(STDOUT_FILENO) && !std::getenv("NO_COLOR");
    if(!platform::parseOptions(arguments,options,error)){std::fprintf(stderr,"ACT3: %s\n",error.c_str());return 2;}
    if(options.version){std::puts("ACT3 " ACT3_VERSION);return 0;}
    if(options.checkAssets){if(!runtime::checkAssets(error))return reportError(error.c_str());std::puts("ACT3 assets OK");return 0;}
    if(options.help){platform::printHelp(color);return 0;}
    Display* display = XOpenDisplay(nullptr);
    if (!display)
        return reportError("Could not connect to the X display.");
    const bool explicitUi=std::find(arguments.begin(),arguments.end(),"--ui")!=arguments.end();
    if(options.launcher&&!options.checkScenes&&(arguments.empty()||explicitUi)){
        if(!platform::showLinuxLauncher(display,options)){XCloseDisplay(display);return 0;}
    }
    std::printf("\n%s  ACT3 / REVERSED ENGINEERS%s\n  %s / soundtrack on\n  Loading the journey...\n\n",
        color?"\033[1;31m":"",color?"\033[0m":"",extra::enabled?"22nd anniversary edition / 16:9":"Party version / 4:3");
    const int result = runWindow(display,options);
    XCloseDisplay(display);
    return result;
}
