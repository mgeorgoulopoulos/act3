#include "linux_display.h"
#include <algorithm>
platform::LinuxDisplay::LinuxDisplay(Display* d):display(d),root(DefaultRootWindow(d)) {
    int screen=DefaultScreen(d);width=oldWidth=DisplayWidth(d,screen);height=oldHeight=DisplayHeight(d,screen);
    oldMMWidth=DisplayWidthMM(d,screen);oldMMHeight=DisplayHeightMM(d,screen);
    int event,error;
    if(!XRRQueryExtension(d,&event,&error))return;
    auto* resources=XRRGetScreenResourcesCurrent(d,root);if(!resources)return;
    auto primary=XRRGetOutputPrimary(d,root);
    for(int pass=0;pass<2&&!previous;++pass)for(int i=0;i<resources->noutput;++i){
        if(pass==0&&resources->outputs[i]!=primary)continue;
        auto* output=XRRGetOutputInfo(d,resources,resources->outputs[i]);
        if(output&&output->connection==RR_Connected&&output->crtc){crtc=output->crtc;previous=XRRGetCrtcInfo(d,resources,crtc);}
        if(output)XRRFreeOutputInfo(output);if(previous)break;
    }
    if(previous){x=previous->x;y=previous->y;width=previous->width;height=previous->height;}
    XRRFreeScreenResources(resources);
}
bool platform::LinuxDisplay::configure(const LaunchOptions& options,std::string& error) {
    if(!options.fullscreen||!options.width||(options.width==width&&options.height==height))return true;
    if(!previous){error="Display mode switching needs XRandR. Use --windowed or the native fullscreen resolution.";return false;}
    auto* resources=XRRGetScreenResourcesCurrent(display,root);if(!resources){error="Could not query display modes.";return false;}
    RRMode selected=0;
    // A cloned output must support the selected mode on every connected output of this CRTC.
    for(int i=0;i<resources->nmode&&!selected;++i){
        auto mode=resources->modes[i];int w=mode.width,h=mode.height;
        if(previous->rotation&(RR_Rotate_90|RR_Rotate_270))std::swap(w,h);
        if(w!=options.width||h!=options.height)continue;
        bool supported=true;
        for(int j=0;j<previous->noutput;++j){
            auto* output=XRRGetOutputInfo(display,resources,previous->outputs[j]);bool found=false;
            if(output){for(int k=0;k<output->nmode;++k)if(output->modes[k]==mode.id)found=true;XRRFreeOutputInfo(output);}
            supported&=found;
        }
        if(supported)selected=mode.id;
    }
    if(!selected){XRRFreeScreenResources(resources);error="Unsupported fullscreen resolution. Choose a supported mode or use --windowed.";return false;}
    int w=std::max(oldWidth,x+options.width),h=std::max(oldHeight,y+options.height);
    if(w!=oldWidth||h!=oldHeight){
        int minW,minH,maxW,maxH;
        if(!XRRGetScreenSizeRange(display,root,&minW,&minH,&maxW,&maxH)||w>maxW||h>maxH){XRRFreeScreenResources(resources);error="Resolution exceeds the X screen limit.";return false;}
        XRRSetScreenSize(display,root,w,h,std::max(1,oldMMWidth*w/oldWidth),std::max(1,oldMMHeight*h/oldHeight));expanded=true;
        XRRFreeScreenResources(resources);resources=XRRGetScreenResourcesCurrent(display,root);
        if(!resources){error="Could not refresh the display configuration.";return false;}
    }
    auto status=XRRSetCrtcConfig(display,resources,crtc,CurrentTime,x,y,selected,previous->rotation,previous->outputs,previous->noutput);
    XRRFreeScreenResources(resources);
    if(status!=Success){error="The display rejected that resolution. Try --windowed.";return false;}
    changed=true;width=options.width;height=options.height;XSync(display,False);return true;
}
platform::LinuxDisplay::~LinuxDisplay(){
    if(changed){
        if(auto* resources=XRRGetScreenResourcesCurrent(display,root)){
            XRRSetCrtcConfig(display,resources,crtc,CurrentTime,previous->x,previous->y,previous->mode,previous->rotation,previous->outputs,previous->noutput);
            XRRFreeScreenResources(resources);
        }
    }
    if(expanded)XRRSetScreenSize(display,root,oldWidth,oldHeight,std::max(1,oldMMWidth),std::max(1,oldMMHeight));
    if(previous)XRRFreeCrtcInfo(previous);
}
