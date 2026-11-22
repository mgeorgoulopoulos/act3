#pragma once
#include <string>
#include <vector>
bool readJpeg(const std::string& path,unsigned& width,unsigned& height,std::vector<unsigned char>& pixels);
