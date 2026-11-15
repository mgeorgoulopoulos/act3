#pragma once

namespace extra {
enum class SceneGrade { Eggplants, Shadow };
void applyColorGrade(int width, int height, SceneGrade scene);
void releaseColorGrade();
}
