#pragma once
#include <string_view>

namespace preview {
inline bool shadowSelected = false;
inline bool sequenceSelected = false;
void selectOption(std::string_view argument);
const char* windowTitle();
bool load();
void drawLoading(int width,int height);
void draw(int width, int height);
void present(int width,int height);
void releasePresentation();
void releaseTextures();
bool finished();
}
