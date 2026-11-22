#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows_launcher.h"
#include "windows_logo.h"
#include "../extra/mode.h"
#include "../recreated/mesh_preview.h"
#include <algorithm>
#include <string>
#include <vector>
namespace {
constexpr COLORREF bg=RGB(17,18,23),panel=RGB(29,31,39),ink=RGB(236,237,241),muted=RGB(155,160,174),red=RGB(223,54,74);
constexpr int extraId=101,originalId=102,resId=103,displayId=104,goId=IDOK;
struct Resolution { int width,height; };
struct Launcher {
    platform::LaunchOptions* options;
    std::vector<Resolution> modes;
    HWND resolution{},display{},extra{},original{};
    HFONT font{},small{},logo{};HBRUSH brush{};HBITMAP figure{};
    bool done=false,accepted=false;float scale=1;int desktopWidth=0,desktopHeight=0;
    int px(int n)const{return int(n*scale+.5f);}
};
void text(HDC dc,HFONT font,COLORREF color,RECT r,const wchar_t* value,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE) {
    SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,value,-1,&r,flags);
}
void addMode(Launcher& s,int w,int h){
    if(w<320 || h<320)return;
    for(auto m:s.modes)if(m.width==w&&m.height==h)return;
    s.modes.push_back({w,h});
}
LRESULT CALLBACK comboProc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    auto original=reinterpret_cast<WNDPROC>(GetPropW(window,L"ACT3ComboProc"));
    auto result=CallWindowProcW(original,window,message,wp,lp);
    if(message==WM_PAINT || message==WM_PRINTCLIENT){
        HDC dc=message==WM_PRINTCLIENT?reinterpret_cast<HDC>(wp):GetDC(window);
        RECT r;GetClientRect(window,&r);int arrow=GetSystemMetrics(SM_CXVSCROLL);
        RECT button{r.right-arrow-2,2,r.right-2,r.bottom-2};
        auto fill=CreateSolidBrush(panel);FillRect(dc,&button,fill);DeleteObject(fill);
        int x=(button.left+button.right)/2,y=(button.top+button.bottom)/2;
        auto pen=CreatePen(PS_SOLID,1,muted);auto old=SelectObject(dc,pen);
        MoveToEx(dc,x-4,y-2,nullptr);LineTo(dc,x,y+2);LineTo(dc,x+4,y-2);
        SelectObject(dc,old);DeleteObject(pen);if(message==WM_PAINT)ReleaseDC(window,dc);
    }
    return result;
}
LRESULT CALLBACK proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    auto* s=reinterpret_cast<Launcher*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE){s=static_cast<Launcher*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}
    if(!s)return DefWindowProcW(window,message,wp,lp);
    switch(message){
    case DM_GETDEFID:return MAKELRESULT(goId,DC_HASDEFID);
    case WM_CLOSE:s->done=true;return 0;
    case WM_ERASEBKGND:return 1;
    case WM_CTLCOLORLISTBOX:case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wp),ink);SetBkColor(reinterpret_cast<HDC>(wp),panel);return reinterpret_cast<LRESULT>(s->brush);
    case WM_COMMAND:{
        int id=LOWORD(wp);
        if(id==IDCANCEL){s->done=true;return 0;}
        if(id==extraId||id==originalId){
            extra::enabled=id==extraId;
            preview::selectOption(extra::enabled?"--extra":"--original");
            InvalidateRect(window,nullptr,FALSE);InvalidateRect(s->extra,nullptr,TRUE);InvalidateRect(s->original,nullptr,TRUE);return 0;
        }
        if(id==displayId && HIWORD(wp)==CBN_SELCHANGE && SendMessageW(s->display,CB_GETCURSEL,0,0)==1){
            for(size_t i=0;i<s->modes.size();++i)if(s->modes[i].width==s->desktopWidth&&s->modes[i].height==s->desktopHeight)SendMessageW(s->resolution,CB_SETCURSEL,i,0);
            return 0;
        }
        if(id==goId){
            auto index=SendMessageW(s->resolution,CB_GETCURSEL,0,0);
            if(index>=0 && size_t(index)<s->modes.size()){
                s->options->width=s->modes[index].width;s->options->height=s->modes[index].height;
                s->options->fullscreen=SendMessageW(s->display,CB_GETCURSEL,0,0)==1;
                s->accepted=s->done=true;
            }return 0;
        }return 0;
    }
    case WM_DRAWITEM:{
        auto& d=*reinterpret_cast<DRAWITEMSTRUCT*>(lp);RECT r=d.rcItem;
        bool selected=(d.CtlID==extraId&&extra::enabled)||(d.CtlID==originalId&&!extra::enabled);
        bool primary=d.CtlID==goId;
        auto fill=CreateSolidBrush(primary?red:selected?RGB(53,31,40):(d.itemState&ODS_SELECTED)?RGB(62,39,49):panel);FillRect(d.hDC,&r,fill);DeleteObject(fill);
        if(selected){auto edge=CreateSolidBrush(red);FrameRect(d.hDC,&r,edge);DeleteObject(edge);}
        if(d.CtlType==ODT_COMBOBOX){
            if(d.itemID!=UINT(-1)){wchar_t value[120]{};SendMessageW(d.hwndItem,CB_GETLBTEXT,d.itemID,reinterpret_cast<LPARAM>(value));
                r.left+=s->px(12);text(d.hDC,s->font,ink,r,value);}
        }else{
            wchar_t value[120]{};GetWindowTextW(d.hwndItem,value,120);
            if(d.CtlID==extraId||d.CtlID==originalId){
                r.left+=s->px(18);RECT title=r;title.bottom=title.top+s->px(40);text(d.hDC,s->font,ink,title,value);
                r.top+=s->px(34);text(d.hDC,s->small,muted,r,d.CtlID==extraId?L"16:9 / Enhanced art, cameras, color":L"4:3 / The restored 2004 production");
            }else text(d.hDC,s->font,ink,r,value,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
        if(d.itemState&ODS_FOCUS){RECT focus=d.rcItem;InflateRect(&focus,-s->px(4),-s->px(4));DrawFocusRect(d.hDC,&focus);}
        return TRUE;
    }
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT all;GetClientRect(window,&all);auto b=CreateSolidBrush(bg);FillRect(dc,&all,b);DeleteObject(b);
        auto label=[&](int x,int y,int w,int h,HFONT font,COLORREF color,const wchar_t* value){RECT r{s->px(x),s->px(y),s->px(x+w),s->px(y+h)};text(dc,font,color,r,value);};
        label(32,24,500,22,s->small,red,L"REVERSED ENGINEERS  /  2004");
        label(116,49,450,76,s->logo,ink,L"ACT3");
        if(s->figure){
            HDC source=CreateCompatibleDC(dc);auto old=SelectObject(source,s->figure);
            BITMAP bitmap{};GetObject(s->figure,sizeof(bitmap),&bitmap);
            BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
            AlphaBlend(dc,s->px(32),s->px(45),bitmap.bmWidth,bitmap.bmHeight,source,0,0,bitmap.bmWidth,bitmap.bmHeight,blend);
            SelectObject(source,old);DeleteDC(source);
        }
        label(32,130,500,25,s->font,muted,L"All rights reversed. Ready when you are.");
        label(32,177,500,20,s->small,muted,L"EDITION");
        label(32,294,240,20,s->small,muted,L"RESOLUTION");label(314,294,240,20,s->small,muted,L"DISPLAY");
        label(32,380,530,24,s->small,muted,extra::enabled?L"16:9  /  Black bars preserve the composition.":L"4:3  /  Black bars preserve the original composition.");
        label(32,470,530,24,s->small,muted,L"Soundtrack on.   Escape exits the demo.");
        EndPaint(window,&ps);return 0;
    }
    }
    return DefWindowProcW(window,message,wp,lp);
}
}
bool platform::showLauncher(HINSTANCE instance,LaunchOptions& options){
    Launcher s{};s.options=&options;
    POINT cursor{};GetCursorPos(&cursor);MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);
    GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTOPRIMARY),&monitor);
    HDC screen=GetDC(nullptr);s.scale=GetDeviceCaps(screen,LOGPIXELSX)/96.f;ReleaseDC(nullptr,screen);
    s.figure=loadLauncherFigure(instance,s.px(84));
    auto font=[&](int height,int weight){return CreateFontW(-s.px(height),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
    s.font=font(16,FW_MEDIUM);s.small=font(12,FW_NORMAL);s.logo=font(70,FW_BOLD);s.brush=CreateSolidBrush(panel);
    DEVMODEW mode{};mode.dmSize=sizeof(mode);
    for(DWORD i=0;EnumDisplaySettingsW(monitor.szDevice,i,&mode);++i)addMode(s,mode.dmPelsWidth,mode.dmPelsHeight);
    int desktopW=monitor.rcMonitor.right-monitor.rcMonitor.left,desktopH=monitor.rcMonitor.bottom-monitor.rcMonitor.top;
    s.desktopWidth=desktopW;s.desktopHeight=desktopH;
    if(!options.width && options.fullscreen){options.width=desktopW;options.height=desktopH;}
    if(!options.width){options.width=desktopW;options.height=desktopH;}
    addMode(s,options.width,options.height);addMode(s,desktopW,desktopH);
    std::sort(s.modes.begin(),s.modes.end(),[](auto a,auto b){return a.width==b.width?a.height<b.height:a.width<b.width;});
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=instance;cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(108));cls.lpszClassName=L"ACT3Launcher";RegisterClassW(&cls);
    RECT r{0,0,s.px(600),s.px(516)};AdjustWindowRectEx(&r,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_CONTROLPARENT);
    HWND window=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"ACT3 / Reversed Engineers",WS_CAPTION|WS_SYSMENU,
        monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-(r.right-r.left))/2,monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-(r.bottom-r.top))/2,
        r.right-r.left,r.bottom-r.top,nullptr,nullptr,instance,&s);
    if(!window){DeleteObject(s.font);DeleteObject(s.small);DeleteObject(s.logo);DeleteObject(s.brush);if(s.figure)DeleteObject(s.figure);return false;}
    if(auto dwm=LoadLibraryExW(L"dwmapi.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)){using Fn=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto set=reinterpret_cast<Fn>(GetProcAddress(dwm,"DwmSetWindowAttribute"));BOOL dark=TRUE;if(set){set(window,20,&dark,sizeof(dark));COLORREF caption=bg,captionText=ink;set(window,35,&caption,sizeof(caption));set(window,36,&captionText,sizeof(captionText));}FreeLibrary(dwm);}
    auto control=[&](const wchar_t* type,const wchar_t* title,DWORD style,int id,int x,int y,int w,int h){
        HWND child=CreateWindowExW(0,type,title,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,s.px(x),s.px(y),s.px(w),s.px(h),window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);
        SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(s.font),FALSE);
        if(auto theme=LoadLibraryExW(L"uxtheme.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)){
            using SetTheme=HRESULT(WINAPI*)(HWND,LPCWSTR,LPCWSTR);
            auto set=reinterpret_cast<SetTheme>(GetProcAddress(theme,"SetWindowTheme"));
            if(set)set(child,L"DarkMode_CFD",nullptr);FreeLibrary(theme);
        }
        return child;
    };
    s.original=control(L"BUTTON",L"Party version",BS_OWNERDRAW,originalId,32,207,260,72);
    s.extra=control(L"BUTTON",L"22nd anniversary edition",BS_OWNERDRAW,extraId,308,207,260,72);
    s.resolution=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,resId,32,323,260,280);
    s.display=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS,displayId,308,323,260,180);
    for(auto child:{s.resolution,s.display}){auto original=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(child,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(comboProc)));SetPropW(child,L"ACT3ComboProc",reinterpret_cast<HANDLE>(original));SendMessageW(child,CB_SETITEMHEIGHT,-1,s.px(32));SendMessageW(child,CB_SETITEMHEIGHT,0,s.px(32));}
    for(size_t i=0;i<s.modes.size();++i){auto m=s.modes[i];auto name=std::to_wstring(m.width)+L" x "+std::to_wstring(m.height);if(m.width==desktopW&&m.height==desktopH)name+=L"  /  native";
        SendMessageW(s.resolution,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));if(m.width==options.width&&m.height==options.height)SendMessageW(s.resolution,CB_SETCURSEL,i,0);}
    for(auto name:{L"Windowed",L"Fullscreen"})SendMessageW(s.display,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
    SendMessageW(s.display,CB_SETCURSEL,options.fullscreen?1:0,0);
    control(L"BUTTON",L"Exit",BS_OWNERDRAW,IDCANCEL,32,424,128,42);
    control(L"BUTTON",L"Go  /  Play ACT3",BS_OWNERDRAW,goId,308,424,260,42);
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(extra::enabled?s.extra:s.original);
    MSG msg{};while(!s.done && GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    DestroyWindow(window);DeleteObject(s.font);DeleteObject(s.small);DeleteObject(s.logo);DeleteObject(s.brush);if(s.figure)DeleteObject(s.figure);return s.accepted;
}
