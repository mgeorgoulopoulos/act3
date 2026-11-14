#include "mesh_preview.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
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

class Reader {
public:
    explicit Reader(std::istream& file)
        : bytes(std::istreambuf_iterator<char>(file), {}) {}

    bool read() const { return chunks(0, bytes.size(), nullptr); }

private:
    std::vector<unsigned char> bytes;

    std::uint16_t word(std::size_t offset) const
    {
        return bytes[offset] | (bytes[offset + 1] << 8);
    }

    std::uint32_t dword(std::size_t offset) const
    {
        return word(offset) | (std::uint32_t(word(offset + 2)) << 16);
    }

    bool chunks(std::size_t pos, std::size_t end, Mesh* mesh,
        unsigned depth = 0) const
    {
        if (depth > 16)
            return false;
        while (pos < end) {
            if (end - pos < 6)
                return false;
            const auto id = word(pos);
            const auto size = dword(pos + 2);
            if (size < 6 || size > end - pos)
                return false;
            const auto next = pos + size;
            pos += 6;
            if (id == 0x4d4d || id == 0x3d3d || id == 0x4100) {
                if (!chunks(pos, next, mesh, depth + 1))
                    return false;
            } else if (id == 0x4000) {
                while (pos < next && bytes[pos] != 0)
                    ++pos;
                Mesh object;
                if (pos == next || !chunks(pos + 1, next, &object, depth + 1))
                    return false;
                if (!object.indices.empty()) {
                    for (auto index : object.indices)
                        if (index >= object.vertices.size())
                            return false;
                    meshes.push_back(std::move(object));
                }
            } else if (mesh && (id == 0x4110 || id == 0x4120)) {
                if (next - pos < 2)
                    return false;
                const auto count = word(pos);
                pos += 2;
                if (id == 0x4110) {
                    if (count > (next - pos) / 12)
                        return false;
                    mesh->vertices.resize(count);
                    for (auto& vertex : mesh->vertices) {
                        for (auto& coordinate : vertex) {
                            const auto bits = dword(pos);
                            std::memcpy(&coordinate, &bits, sizeof(bits));
                            if (!std::isfinite(coordinate))
                                return false;
                            pos += 4;
                        }
                    }
                } else {
                    if (count > (next - pos) / 8)
                        return false;
                    mesh->indices.clear();
                    for (unsigned face = 0; face < count; ++face, pos += 8)
                        for (unsigned corner = 0; corner < 3; ++corner)
                            mesh->indices.push_back(word(pos + 2 * corner));
                }
            }
            pos = next;
        }
        return true;
    }
};
}

bool preview::load()
{
    meshes.clear();
    std::ifstream file(ACT3_MESH_PATH, std::ios::binary);
    if (!file || !Reader(file).read() || meshes.empty())
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
