cmake_minimum_required(VERSION 3.16)
project(ACT3 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The initial ACT3 window target requires Windows.")
endif()

add_executable(act3 WIN32 platform/windows_window.cpp)
target_compile_features(act3 PRIVATE cxx_std_11)
target_link_libraries(act3 PRIVATE opengl32 gdi32 user32)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT act3)
