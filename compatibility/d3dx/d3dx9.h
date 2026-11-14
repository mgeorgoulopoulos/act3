#pragma once

#include <cstdint>

using FLOAT = float;

#ifdef _WIN32
using DWORD = unsigned long;
#else
using DWORD = std::uint32_t;
#endif
