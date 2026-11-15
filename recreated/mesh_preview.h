#pragma once

namespace preview {
inline bool shadowSelected = false;
inline bool sequenceSelected = false;
bool load();
void draw(int width, int height);
void releaseTextures();
}
