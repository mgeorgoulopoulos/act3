#pragma once

namespace extra {
enum class SceneGrade { Eggplants, Shadow };
void captureShadowFigure();
void applyColorGrade(int width, int height, SceneGrade scene, unsigned time = 0);
void releaseColorGrade();
}
