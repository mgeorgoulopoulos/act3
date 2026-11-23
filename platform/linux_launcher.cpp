#include "linux_launcher.h"
#include "linux_display.h"
#include "../extra/mode.h"
#include "../recreated/assets.h"
#include "../recreated/mesh_preview.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <png.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
constexpr int width=760,height=600;
struct Rect { int x,y,w,h; bool contains(int px,int py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; } };
const Rect controls[]={{32,240,340,86},{388,240,340,86},{32,380,340,44},{388,380,340,44},{32,490,130,46},{388,490,340,46}};
struct Mode { int w,h; bool operator==(const Mode& other)const{return w==other.w&&h==other.h;} };
struct Launcher {
    Display* display{}; Window window{}; Pixmap canvas{}; GC gc{}; Visual* visual{}; int depth{};
    XFontStruct *font{},*small{}; XImage* figure{};
    unsigned long bg{},panel{},ink{},muted{},red{},edge{},selected{},hoverColor{};
    float scale=1; Mode native{}; std::vector<Mode> supported,modes;
    int resolution{},focus{},hover=-1,pressed=-1,popup=-1,choice{},scroll{};
    bool anniversary{},fullscreen=true,accepted{},done{},exists=true;
    int px(float value) const{return int(std::lround(value*scale));}
    int count()const{return popup==2?int(modes.size()):2;}
    Rect menu()const{const auto r=controls[popup];const int h=std::min(6,count())*34;return {r.x,r.y-h-6,r.w,h};}
};
unsigned long color(Launcher& s,unsigned rgb){
    XColor c{};c.red=((rgb>>16)&255)*257;c.green=((rgb>>8)&255)*257;c.blue=(rgb&255)*257;
    return XAllocColor(s.display,DefaultColormap(s.display,DefaultScreen(s.display)),&c)?c.pixel:BlackPixel(s.display,DefaultScreen(s.display));
}
void fill(Launcher& s,Rect r,unsigned long c){XSetForeground(s.display,s.gc,c);XFillRectangle(s.display,s.canvas,s.gc,s.px(r.x),s.px(r.y),s.px(r.w),s.px(r.h));}
void border(Launcher& s,Rect r,unsigned long c){XSetForeground(s.display,s.gc,c);XDrawRectangle(s.display,s.canvas,s.gc,s.px(r.x),s.px(r.y),s.px(r.w)-1,s.px(r.h)-1);}
void text(Launcher& s,int x,int baseline,const std::string& value,unsigned long c,bool small=false){
    auto* font=small?s.small:s.font;XSetFont(s.display,s.gc,font->fid);XSetForeground(s.display,s.gc,c);
    XDrawString(s.display,s.canvas,s.gc,s.px(x),s.px(baseline),value.c_str(),int(value.size()));
}
void centered(Launcher& s,int baseline,const std::string& value,unsigned long c,bool small=false){
    auto* font=small?s.small:s.font;const int size=XTextWidth(font,value.c_str(),int(value.size()));
    text(s,int((s.px(width)-size)/(2*s.scale)),baseline,value,c,small);
}
void line(Launcher& s,int x,int y,const std::vector<std::array<int,2>>& points){
    std::vector<XPoint> path;for(auto p:points)path.push_back({short(s.px(x+p[0])),short(s.px(y+p[1]))});
    XDrawLines(s.display,s.canvas,s.gc,path.data(),int(path.size()),CoordModeOrigin);
}
void title(Launcher& s){
    // A small geometric wordmark, independent of installed desktop font families.
    const int x=318,y=78;XSetForeground(s.display,s.gc,s.ink);
    XSetLineAttributes(s.display,s.gc,std::max(1,s.px(7)),LineSolid,CapRound,JoinRound);
    line(s,x,y,{{0,64},{23,0},{46,64}});line(s,x,y,{{9,42},{37,42}});
    line(s,x+66,y,{{43,5},{34,0},{12,0},{2,12},{0,32},{2,52},{12,64},{34,64},{43,59}});
    line(s,x+125,y,{{0,0},{46,0}});line(s,x+125,y,{{23,0},{23,64}});
    line(s,x+190,y,{{0,0},{32,0},{43,10},{43,22},{31,32},{11,32},{31,32},{43,43},{43,54},{32,64},{0,64}});
    XSetLineAttributes(s.display,s.gc,1,LineSolid,CapButt,JoinMiter);
}
unsigned long channel(unsigned value,unsigned long mask){
    if(!mask)return 0;unsigned shift=0;while((mask&1)==0){mask>>=1;++shift;}
    return ((value*mask+127)/255)<<shift;
}
XImage* loadFigure(Launcher& s){
    const auto path=runtime::textureRoot()/"launcher-eggplant.png";
    png_image image{};image.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&image,path.c_str()))return nullptr;
    image.format=PNG_FORMAT_RGBA;std::vector<unsigned char> rgba(PNG_IMAGE_SIZE(image));
    if(!png_image_finish_read(&image,nullptr,rgba.data(),0,nullptr)){png_image_free(&image);return nullptr;}
    const int h=s.px(122),w=int(double(image.width)*h/image.height);
    auto* result=XCreateImage(s.display,s.visual,s.depth,ZPixmap,0,nullptr,w,h,32,0);
    if(!result){png_image_free(&image);return nullptr;}
    result->data=static_cast<char*>(std::calloc(result->bytes_per_line,h));
    if(!result->data){XDestroyImage(result);png_image_free(&image);return nullptr;}
    const float background[]={17,18,23};
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        const float u=std::clamp((x+.5f)*image.width/w-.5f,0.f,float(image.width-1));
        const float v=std::clamp((y+.5f)*image.height/h-.5f,0.f,float(image.height-1));
        const int x0=int(u),y0=int(v),x1=std::min(x0+1,int(image.width)-1),y1=std::min(y0+1,int(image.height)-1);
        const float fx=u-x0,fy=v-y0;unsigned rgb[3]{};
        for(int c=0;c<3;++c){
            const auto sample=[&](int a,int b){const auto i=4*(b*image.width+a);const float alpha=rgba[i+3]/255.f;return rgba[i+c]*alpha+background[c]*(1-alpha);};
            rgb[c]=unsigned(std::lround((1-fy)*((1-fx)*sample(x0,y0)+fx*sample(x1,y0))+fy*((1-fx)*sample(x0,y1)+fx*sample(x1,y1))));
        }
        XPutPixel(result,x,y,channel(rgb[0],s.visual->red_mask)|channel(rgb[1],s.visual->green_mask)|channel(rgb[2],s.visual->blue_mask));
    }
    png_image_free(&image);return result;
}
void add(std::vector<Mode>& modes,Mode mode){if(mode.w>=320&&mode.h>=320&&std::find(modes.begin(),modes.end(),mode)==modes.end())modes.push_back(mode);}
std::vector<Mode> displayModes(Launcher& s,const platform::LinuxDisplay& monitor){
    std::vector<Mode> result;add(result,s.native);
    int event,error;if(!XRRQueryExtension(s.display,&event,&error))return result;
    auto* resources=XRRGetScreenResourcesCurrent(s.display,DefaultRootWindow(s.display));
    if(!resources)return result;
    for(int i=0;i<resources->noutput;++i){
        auto* output=XRRGetOutputInfo(s.display,resources,resources->outputs[i]);
        if(output&&output->connection==RR_Connected&&output->crtc){
            auto* crtc=XRRGetCrtcInfo(s.display,resources,output->crtc);
            if(crtc&&crtc->x==monitor.x&&crtc->y==monitor.y){
                for(int j=0;j<output->nmode;++j)for(int k=0;k<resources->nmode;++k)if(output->modes[j]==resources->modes[k].id){
                    int w=resources->modes[k].width,h=resources->modes[k].height;
                    bool supported=true;
                    for(int clone=0;clone<crtc->noutput;++clone){
                        auto* info=XRRGetOutputInfo(s.display,resources,crtc->outputs[clone]);bool found=false;
                        if(info){for(int m=0;m<info->nmode;++m)if(info->modes[m]==resources->modes[k].id)found=true;XRRFreeOutputInfo(info);}supported&=found;
                    }
                    if(crtc->rotation&(RR_Rotate_90|RR_Rotate_270))std::swap(w,h);if(supported)add(result,{w,h});
                }
            }
            if(crtc)XRRFreeCrtcInfo(crtc);
        }
        if(output)XRRFreeOutputInfo(output);
    }
    XRRFreeScreenResources(resources);return result;
}
void rebuildModes(Launcher& s,Mode previous){
    s.modes=s.supported;
    if(!s.fullscreen)for(auto m:{Mode{800,600},Mode{1024,768},Mode{1280,720},Mode{1280,800},Mode{1600,900},Mode{1920,1080},Mode{2560,1440}})
        if(m.w<=s.native.w&&m.h<=s.native.h)add(s.modes,m);
    std::sort(s.modes.begin(),s.modes.end(),[](Mode a,Mode b){return a.w*a.h==b.w*b.h?a.w<b.w:a.w*a.h<b.w*b.h;});
    auto found=std::find(s.modes.begin(),s.modes.end(),previous);
    if(found==s.modes.end())found=std::find(s.modes.begin(),s.modes.end(),s.native);
    s.resolution=int(found-s.modes.begin());
}
std::string modeLabel(const Launcher& s,int index){auto m=s.modes[index];return std::to_string(m.w)+" x "+std::to_string(m.h)+(m==s.native?"  /  native":"");}
void ensureVisible(Launcher& s){const int rows=std::min(6,s.count());s.scroll=std::clamp(s.scroll,std::max(0,s.choice-rows+1),s.choice);s.scroll=std::clamp(s.scroll,0,std::max(0,s.count()-rows));}
void draw(Launcher& s){
    fill(s,{0,0,width,height},s.bg);
    centered(s,28,"REVERSED ENGINEERS  /  2004",s.red,true);
    if(s.figure)XPutImage(s.display,s.canvas,s.gc,s.figure,0,0,s.px(202),s.px(47),s.figure->width,s.figure->height);
    title(s);centered(s,185,"All rights reversed. Ready when you are.",s.muted);
    text(s,32,222,"EDITION",s.muted,true);
    for(int i=0;i<6;++i){
        const bool selected=i<2&&s.anniversary==bool(i);
        fill(s,controls[i],i==5?s.red:selected?s.selected:(s.hover==i?s.hoverColor:s.panel));
        border(s,controls[i],selected?s.red:s.edge);
        if(i<2){
            text(s,controls[i].x+18,272,i?"22nd anniversary edition":"Party version",s.ink);
            text(s,controls[i].x+18,302,i?"16:9 / HD definition, cameras, color":"4:3 / The restored 2004 production",s.muted,true);
            if(selected){XSetForeground(s.display,s.gc,s.red);XFillArc(s.display,s.canvas,s.gc,s.px(controls[i].x+316),s.px(254),s.px(8),s.px(8),0,360*64);}
        }else if(i==2||i==3){
            text(s,controls[i].x+14,407,i==2?modeLabel(s,s.resolution):(s.fullscreen?"Fullscreen":"Windowed"),s.ink);
            XSetForeground(s.display,s.gc,s.muted);line(s,controls[i].x+314,399,{{0,0},{5,5},{10,0}});
        }else{
            const std::string value=i==5?"Go  /  Play ACT3":"Exit";
            const int tw=XTextWidth(s.font,value.c_str(),int(value.size()));
            text(s,controls[i].x+int((s.px(controls[i].w)-tw)/(2*s.scale)),520,value,s.ink);
        }
        if(s.focus==i){
            const auto r=controls[i];XSetLineAttributes(s.display,s.gc,1,LineOnOffDash,CapButt,JoinMiter);
            border(s,{r.x+4,r.y+4,r.w-8,r.h-8},s.muted);XSetLineAttributes(s.display,s.gc,1,LineSolid,CapButt,JoinMiter);
        }
    }
    text(s,32,363,"RESOLUTION",s.muted,true);text(s,388,363,"DISPLAY",s.muted,true);
    text(s,32,456,s.anniversary?"16:9  /  Black bars preserve the composition.":"4:3  /  Black bars preserve the original composition.",s.muted,true);
    text(s,32,574,"Soundtrack on.  Escape exits the demo.",s.muted,true);
    if(s.popup>=0){
        const auto r=s.menu();fill(s,r,s.panel);border(s,r,s.edge);const int rows=std::min(6,s.count());
        for(int i=0;i<rows;++i){const int index=s.scroll+i;
            if(index==s.choice)fill(s,{r.x+2,r.y+2+i*34,r.w-4,32},s.selected);
            text(s,r.x+14,r.y+23+i*34,s.popup==2?modeLabel(s,index):(index?"Fullscreen":"Windowed"),s.ink);
        }
        if(s.count()>rows){fill(s,{r.x+r.w-6,r.y+4,2,r.h-8},s.edge);fill(s,{r.x+r.w-6,r.y+4+s.scroll*(r.h-8)/s.count(),2,std::max(10,rows*(r.h-8)/s.count())},s.red);}
    }
    XCopyArea(s.display,s.canvas,s.window,s.gc,0,0,s.px(width),s.px(height),0,0);XFlush(s.display);
}
void choose(Launcher& s){
    if(s.popup==2)s.resolution=s.choice;
    else if(s.popup==3){s.fullscreen=s.choice==1;rebuildModes(s,s.fullscreen?s.native:s.modes[s.resolution]);}
    s.popup=-1;
}
void activate(Launcher& s,int id){
    if(id<2)s.anniversary=id==1;
    else if(id==2||id==3){
        if(s.popup==id){s.popup=-1;return;}
        s.popup=id;s.choice=id==2?s.resolution:int(s.fullscreen);s.scroll=0;ensureVisible(s);
    }else{s.done=true;s.accepted=id==5;}
}
XFontStruct* loadFont(Display* d,int pixels){
    const std::string name=std::string("-*-helvetica-medium-r-normal--")+std::to_string(pixels)+"-*-*-*-*-*-iso8859-1";
    auto* font=XLoadQueryFont(d,name.c_str());return font?font:XLoadQueryFont(d,"fixed");
}
}

bool platform::showLinuxLauncher(Display* display,LaunchOptions& options){
    Launcher s;s.display=display;s.visual=DefaultVisual(display,DefaultScreen(display));s.depth=DefaultDepth(display,DefaultScreen(display));
    LinuxDisplay monitor(display);s.native={monitor.width,monitor.height};
    s.scale=std::min(1.f,std::min(monitor.width/800.f,monitor.height/640.f));
    s.anniversary=extra::enabled;s.fullscreen=options.fullscreen;s.supported=displayModes(s,monitor);
    rebuildModes(s,options.width?Mode{options.width,options.height}:s.native);
    s.bg=color(s,0x111217);s.panel=color(s,0x1d1f27);s.ink=color(s,0xecedf1);s.muted=color(s,0x9ba0ae);
    s.red=color(s,0xdf364a);s.edge=color(s,0x343842);s.selected=color(s,0x351f28);s.hoverColor=color(s,0x282b35);
    s.window=XCreateSimpleWindow(display,DefaultRootWindow(display),monitor.x+(monitor.width-s.px(width))/2,
        monitor.y+(monitor.height-s.px(height))/2,s.px(width),s.px(height),0,s.bg,s.bg);
    s.gc=XCreateGC(display,s.window,0,nullptr);s.canvas=XCreatePixmap(display,s.window,s.px(width),s.px(height),s.depth);
    s.font=loadFont(display,std::max(12,s.px(18)));s.small=loadFont(display,std::max(10,s.px(14)));
    if(!s.font||!s.small){std::fputs("ACT3: Could not load X11 launcher fonts\n",stderr);s.done=true;}
    s.figure=loadFigure(s);
    XStoreName(display,s.window,"ACT3 / Reversed Engineers");XClassHint cls{const_cast<char*>("act3-launcher"),const_cast<char*>("ACT3")};XSetClassHint(display,s.window,&cls);
    XSizeHints hints{};hints.flags=PMinSize|PMaxSize;hints.min_width=hints.max_width=s.px(width);hints.min_height=hints.max_height=s.px(height);XSetWMNormalHints(display,s.window,&hints);
    const Atom protocols=XInternAtom(display,"WM_PROTOCOLS",False),close=XInternAtom(display,"WM_DELETE_WINDOW",False);
    Atom closeProtocol=close;XSetWMProtocols(display,s.window,&closeProtocol,1);
    XWMHints wm{};wm.flags=InputHint;wm.input=True;XSetWMHints(display,s.window,&wm);
    Atom type=XInternAtom(display,"_NET_WM_WINDOW_TYPE_DIALOG",False);XChangeProperty(display,s.window,XInternAtom(display,"_NET_WM_WINDOW_TYPE",False),XA_ATOM,32,PropModeReplace,reinterpret_cast<unsigned char*>(&type),1);
    const char dark[]="dark";XChangeProperty(display,s.window,XInternAtom(display,"_GTK_THEME_VARIANT",False),XInternAtom(display,"UTF8_STRING",False),8,PropModeReplace,reinterpret_cast<const unsigned char*>(dark),4);
    XSelectInput(display,s.window,ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|LeaveWindowMask|FocusChangeMask|StructureNotifyMask);
    const Cursor cursor=XCreateFontCursor(display,XC_left_ptr);XDefineCursor(display,s.window,cursor);
    if(!s.done)XMapWindow(display,s.window);
    while(!s.done){
        XEvent event{};XNextEvent(display,&event);bool repaint=true;
        switch(event.type){
        case Expose:break;
        case MotionNotify:{
            const int x=int(event.xmotion.x/s.scale),y=int(event.xmotion.y/s.scale);s.hover=-1;
            for(int i=0;i<6;++i)if(controls[i].contains(x,y))s.hover=i;
            if(s.popup>=0){const auto r=s.menu();if(r.contains(x,y))s.choice=std::min(s.count()-1,s.scroll+(y-r.y)/34);}
            break;
        }
        case LeaveNotify:s.hover=-1;break;
        case FocusOut:s.popup=-1;s.pressed=-1;break;
        case ButtonPress:{
            const int x=int(event.xbutton.x/s.scale),y=int(event.xbutton.y/s.scale);
            if(event.xbutton.button==Button4||event.xbutton.button==Button5){
                if(s.popup>=0){s.choice=std::clamp(s.choice+(event.xbutton.button==Button4?-1:1),0,s.count()-1);ensureVisible(s);}
                else if(controls[2].contains(x,y)){s.resolution=std::clamp(s.resolution+(event.xbutton.button==Button4?-1:1),0,int(s.modes.size())-1);}break;
            }
            if(event.xbutton.button!=Button1)break;
            if(s.popup>=0){const auto r=s.menu();if(r.contains(x,y)){s.choice=std::min(s.count()-1,s.scroll+(y-r.y)/34);choose(s);s.pressed=-1;break;}s.popup=-1;}
            s.pressed=-1;for(int i=0;i<6;++i)if(controls[i].contains(x,y)){s.pressed=s.focus=i;break;}break;
        }
        case ButtonRelease:{
            if(event.xbutton.button!=Button1)break;const int x=int(event.xbutton.x/s.scale),y=int(event.xbutton.y/s.scale);
            if(s.pressed>=0&&controls[s.pressed].contains(x,y))activate(s,s.pressed);s.pressed=-1;break;
        }
        case KeyPress:{
            const auto key=XLookupKeysym(&event.xkey,0);
            if(key==XK_Escape){if(s.popup>=0)s.popup=-1;else s.done=true;break;}
            if(key==XK_Tab){s.popup=-1;s.focus=(s.focus+((event.xkey.state&ShiftMask)?5:1))%6;break;}
            if(s.popup>=0){
                if(key==XK_Return||key==XK_KP_Enter||key==XK_space)choose(s);
                else if(key==XK_Up||key==XK_Down||key==XK_Page_Up||key==XK_Page_Down||key==XK_Home||key==XK_End){
                    if(key==XK_Home)s.choice=0;else if(key==XK_End)s.choice=s.count()-1;
                    else s.choice=std::clamp(s.choice+(key==XK_Up?-1:key==XK_Down?1:key==XK_Page_Up?-6:6),0,s.count()-1);ensureVisible(s);
                }break;
            }
            if(key==XK_Return||key==XK_KP_Enter||key==XK_space)activate(s,s.focus);
            else if((key==XK_Left||key==XK_Right)&&s.focus<2){s.focus=key==XK_Left?0:1;s.anniversary=s.focus==1;}
            else if((key==XK_Up||key==XK_Down)&&s.focus>=2&&s.focus<=3)activate(s,s.focus);
            break;
        }
        case ClientMessage:if(event.xclient.message_type==protocols&&Atom(event.xclient.data.l[0])==close)s.done=true;break;
        case DestroyNotify:s.exists=false;s.done=true;break;
        default:repaint=false;break;
        }
        if(repaint&&!s.done)draw(s);
    }
    if(s.accepted){const auto mode=s.modes[s.resolution];options.width=mode.w;options.height=mode.h;options.fullscreen=s.fullscreen;preview::selectOption(s.anniversary?"--extra":"--original");}
    if(s.figure)XDestroyImage(s.figure);if(s.font)XFreeFont(display,s.font);if(s.small)XFreeFont(display,s.small);
    XFreeCursor(display,cursor);XFreePixmap(display,s.canvas);XFreeGC(display,s.gc);if(s.exists)XDestroyWindow(display,s.window);
    XFlush(display);return s.accepted;
}
