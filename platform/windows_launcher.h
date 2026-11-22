#pragma once
#include "launch_options.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace platform { bool showLauncher(HINSTANCE instance,LaunchOptions& options); }
