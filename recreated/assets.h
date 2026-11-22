#pragma once
#include <filesystem>
#include <string>
namespace runtime {
const std::filesystem::path& assetRoot();
std::filesystem::path textureRoot();
bool checkAssets(std::string& error);
}
