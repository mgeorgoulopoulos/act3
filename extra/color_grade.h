#pragma once

namespace extra {
enum class SceneGrade { Eggplants, Shadow, GreetzBackground, Greetz, Space, Credits };
void captureShadowFigure();
void captureSpaceBackdrop(unsigned time);
void captureSpaceFigures(unsigned time);
void gradeSpaceReveal(unsigned time);
void gradeGreetzBackground();
void gradeGreetzSurface();
void applyColorGrade(int width, int height, SceneGrade scene, unsigned time = 0);
void releaseColorGrade();
}
