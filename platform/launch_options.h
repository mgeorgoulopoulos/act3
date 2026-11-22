#pragma once
#include <string>
#include <vector>
namespace platform {
inline bool verbose=false;
struct LaunchOptions {
    int width=0,height=0;
    bool fullscreen=true,launcher=true,help=false;
};
bool parseOptions(const std::vector<std::string>& arguments,LaunchOptions& options,std::string& error);
void printHelp(bool color);
}
