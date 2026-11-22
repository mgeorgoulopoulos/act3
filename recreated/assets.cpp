#include "assets.h"
#include <fstream>
#include <vector>
#include <charconv>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif
namespace {
std::filesystem::path executableDirectory(){
#ifdef _WIN32
    std::vector<wchar_t> path(32768);
    const auto size=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
    if(size&&size<path.size())return std::filesystem::path(std::wstring(path.data(),size)).parent_path();
#else
    std::vector<char> path(32768);
    auto size=readlink("/proc/self/exe",path.data(),path.size());
    if(size>0&&static_cast<size_t>(size)<path.size())return std::filesystem::path(std::string(path.data(),size)).parent_path();
#endif
    return {};
}
bool containsAssets(const std::filesystem::path& root){
    std::error_code error;
    return std::filesystem::is_regular_file(root/"data/geometry/eggplant.3ds",error) &&
        std::filesystem::is_regular_file(root/"data/Universus.mp3",error);
}
}
const std::filesystem::path& runtime::assetRoot(){
    static const auto root=[](){
        const auto executable=executableDirectory();
        std::error_code error;const auto working=std::filesystem::current_path(error);
        for(const auto& candidate:{executable,executable/"assets",executable.parent_path()/"assets",working,working/"assets"})
            if(containsAssets(candidate))return candidate;
        return executable;
    }();
    return root;
}
std::filesystem::path runtime::textureRoot(){return assetRoot()/"data/textures";}
bool runtime::checkAssets(std::string& error){
    std::ifstream manifest(assetRoot()/"asset-manifest.txt");
    if(!manifest){error="Missing asset-manifest.txt. Keep the complete extracted package with the executable.";return false;}
    size_t count=0;
    for(std::string line;std::getline(manifest,line);){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        auto separator=line.find('|');
        if(separator==std::string::npos){error="Malformed asset manifest.";return false;}
        std::uintmax_t expected=0;auto parsed=std::from_chars(line.data(),line.data()+separator,expected);
        if(parsed.ec!=std::errc{}||parsed.ptr!=line.data()+separator){error="Malformed asset size.";return false;}
        auto relative=std::filesystem::u8path(line.substr(separator+1));
        if(relative.is_absolute()){error="Invalid absolute asset path.";return false;}
        for(const auto& component:relative)if(component==".."){error="Invalid asset path traversal.";return false;}
        std::error_code ec;auto size=std::filesystem::file_size(assetRoot()/relative,ec);
        if(ec||size!=expected){error="Missing or incorrect asset: "+relative.generic_u8string();return false;}
        ++count;
    }
    if(count<10){error="Incomplete asset manifest.";return false;}
    return true;
}
