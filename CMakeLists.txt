cmake_minimum_required(VERSION 3.16)
project(ACT3 LANGUAGES CXX)

if(WIN32)
    add_executable(act3 WIN32 platform/windows_window.cpp)
    target_link_libraries(act3 PRIVATE opengl32 gdi32 user32)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT act3)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX)
    add_executable(act3 platform/linux_window.cpp)
    target_link_libraries(act3 PRIVATE X11::X11 OpenGL::OpenGL OpenGL::GLX)
else()
    message(FATAL_ERROR "ACT3 supports Windows and Linux.")
endif()

target_compile_features(act3 PRIVATE cxx_std_11)
target_sources(act3 PRIVATE recreated/mesh_preview.cpp compatibility/d3d9/indexed_draw.cpp)
set(ACT3_GEOMETRY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../archived/distribution/ACT3/data/geometry"
    CACHE PATH "Directory containing the original ACT3 geometry")
add_executable(act3_export_eggplant recreated/export_eggplant.cpp 3ds.cpp)
target_include_directories(act3_export_eggplant PRIVATE compatibility/d3dx)
target_include_directories(act3 PRIVATE compatibility/d3dx)
set(stem "${CMAKE_CURRENT_BINARY_DIR}/assets/stem.mesh")
set(egg "${CMAKE_CURRENT_BINARY_DIR}/assets/egg.mesh")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command(OUTPUT "${stem}" "${egg}"
    COMMAND act3_export_eggplant "${ACT3_GEOMETRY_DIR}/eggplant.3ds" "${stem}" "${egg}"
    DEPENDS act3_export_eggplant "${ACT3_GEOMETRY_DIR}/eggplant.3ds"
    VERBATIM)
add_custom_target(act3_geometry DEPENDS "${stem}" "${egg}")
add_dependencies(act3 act3_geometry)
target_compile_definitions(act3 PRIVATE ACT3_STEM_PATH="${stem}" ACT3_EGG_PATH="${egg}")
