#pragma once

namespace extra {
enum class SceneGrade { Eggplants, Shadow, GreetzBackground, Greetz, Space };
void captureShadowFigure();
void gradeGreetzBackground();
void gradeGreetzSurface();
void applyColorGrade(int width, int height, SceneGrade scene, unsigned time = 0);
void releaseColorGrade();
}
