#include "mesh_preview.h"
#include "../fvf.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>

namespace {
using Vertex = std::array<float, 3>;
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};
std::vector<Mesh> meshes;
Vertex center{};
float radius = 0;

bool readMesh(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    unsigned short count;
    if (!file.read(reinterpret_cast<char*>(&count), sizeof(count)) || !count)
        return false;
    std::vector<COMMON_VERTEX> vertices(count);
    if (!file.read(reinterpret_cast<char*>(vertices.data()),
                   vertices.size() * sizeof(COMMON_VERTEX)) ||
        !file.read(reinterpret_cast<char*>(&count), sizeof(count)) || !count || count % 3)
        return false;
    Mesh mesh;
    mesh.indices.resize(count);
    if (!file.read(reinterpret_cast<char*>(mesh.indices.data()),
                   mesh.indices.size() * sizeof(unsigned short)))
        return false;
    for (auto index : mesh.indices)
        if (index >= vertices.size())
            return false;
    for (const auto& v : vertices)
        mesh.vertices.push_back({{v.x, v.y, v.z}});
    meshes.push_back(std::move(mesh));
    return true;
}
}

bool preview::load()
{
    meshes.clear();
    if (!readMesh(ACT3_STEM_PATH) || !readMesh(ACT3_EGG_PATH))
        return false;
    Vertex low = meshes.front().vertices.front();
    Vertex high = low;
    for (const auto& mesh : meshes)
        for (const auto& vertex : mesh.vertices)
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = (std::min)(low[axis], vertex[axis]);
                high[axis] = (std::max)(high[axis], vertex[axis]);
            }
    radius = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        center[axis] = (high[axis] + low[axis]) * 0.5f;
        const float extent = high[axis] - center[axis];
        radius += extent * extent;
    }
    radius = std::sqrt(radius);
    return radius > 0 && std::isfinite(radius);
}

void preview::draw(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    const double aspect = double(width) / height;
    const double x = aspect > 1 ? aspect : 1;
    const double y = aspect < 1 ? 1 / aspect : 1;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-x, x, -y, y, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(-65, 1, 0, 0);
    glRotatef(25, 0, 0, 1);
    const float scale = 0.86f / radius;
    glScalef(scale, scale, scale);
    glTranslatef(-center[0], -center[1], -center[2]);
    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.79f, 0.57f, 1.0f);
    for (const auto& mesh : meshes) {
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), mesh.vertices.data());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()),
            GL_UNSIGNED_SHORT, mesh.indices.data());
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisableClientState(GL_VERTEX_ARRAY);
}
