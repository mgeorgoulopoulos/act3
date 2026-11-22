#pragma once
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <string>
#include "launch_options.h"
namespace platform {
// Restore the current output mode when the host exits, including initialization failures.
class LinuxDisplay {
    Display* display;Window root;RRCrtc crtc=0;XRRCrtcInfo* previous=nullptr;
    bool changed=false,expanded=false;int oldWidth,oldHeight,oldMMWidth,oldMMHeight;
public:
    int x=0,y=0,width,height;
    explicit LinuxDisplay(Display* d);
    ~LinuxDisplay();
    bool configure(const LaunchOptions& options,std::string& error);
};
}
