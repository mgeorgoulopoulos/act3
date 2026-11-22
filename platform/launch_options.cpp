#include "launch_options.h"
#include "../recreated/mesh_preview.h"
#include <charconv>
#include <cstdio>
namespace {
bool dimension(const char* begin,const char* end,int& value) {
    auto result=std::from_chars(begin,end,value);
    return result.ec==std::errc{} && result.ptr==end && value>=320 && value<=16384;
}
}
bool platform::parseOptions(const std::vector<std::string>& args,LaunchOptions& o,std::string& error) {
    preview::selectOption("--demo");preview::selectOption("--original");
    for(const auto& a:args) {
        if(a=="--help" || a=="-h") o.help=true;
        else if(a=="--version") o.version=true;
        else if(a=="--check-scenes"){o.checkScenes=true;o.launcher=false;}
        else if(a=="--check-assets") o.checkAssets=true;
        else if(a=="--verbose") verbose=true;
        else if(a=="--no-ui") o.launcher=false;
        else if(a=="--fullscreen") o.fullscreen=true;
        else if(a=="--windowed") o.fullscreen=false;
        else if(a.rfind("--resolution=",0)==0) {
            auto x=a.find('x',13);
            if(x==std::string::npos || !dimension(a.data()+13,a.data()+x,o.width) ||
                !dimension(a.data()+x+1,a.data()+a.size(),o.height)) {
                error="Invalid resolution. Use --resolution=1920x1080 (320..16384 per axis).";return false;
            }
        } else if(a=="--demo" || a=="--extra" || a=="--original" || a=="--space" ||
            a=="--shadow" || a=="--blob" || a=="--credits" || a=="--sequence") {
            preview::selectOption(a);
            if(a!="--demo" && a!="--extra" && a!="--original") o.launcher=false;
        } else { error="Unknown option: "+a+". Use --help to see available options.";return false; }
    }
    return true;
}
void platform::printHelp(bool color) {
    const char* title=color?"\033[1;31m":"";const char* reset=color?"\033[0m":"";
    std::printf("\n%s  A C T 3%s\n  Reversed Engineers / 2004\n\n"
        "  A restored demoscene journey. Music included.\n\n"
        "  act3 [options]\n\n"
        "  EDITION\n"
        "    --extra              22nd anniversary edition, 16:9\n"
        "    --original           Party version, 4:3 (default)\n\n"
        "  DISPLAY\n"
        "    --resolution=WxH     Resolution, e.g. 2560x1440 (default: current desktop)\n"
        "    --windowed           Resizable window\n"
        "    --fullscreen         Fullscreen on the current display (default)\n"
        "    --no-ui              Start directly; skip the Windows launcher\n\n"
        "  PREVIEW\n"
        "    --space --shadow --blob --credits --sequence\n"
        "                         Play one part instead of the full demo\n\n"
        "  --version              Show the player version\n"
        "  --check-assets         Verify the complete asset bundle\n"
        "  --check-scenes         Load scenes in OpenGL, without audio/playback\n"
        "  --verbose              Show resource diagnostics\n"
        "  --help, -h             Show this guide\n\n"
        "  Black bars preserve 16:9 / 4:3. Escape exits playback.\n"
        "  Example: ./act3 --extra --fullscreen --resolution=2560x1440\n\n",title,reset);
}
