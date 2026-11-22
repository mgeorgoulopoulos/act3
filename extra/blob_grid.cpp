#include "blob_grid.h"
#include "mode.h"
#include "model.h"
#include <vector>
#include <algorithm>
namespace {
std::vector<unsigned char> visited;
bool outside(const MeshIso& m,int x,int y,int z) {
    return x<0 || y<0 || z<0 || x>m.xCells || y>m.yCells || z>m.zCells;
}
size_t index(const MeshIso& m,int x,int y,int z) {
    return size_t(x)+size_t(m.xCells+1)*(y+size_t(m.yCells+1)*z);
}
}
int extra::blobResolution() { return enabled ? 48 : 40; }
void extra::beginBlobGrid(const MeshIso& m) {
    visited.assign(size_t(m.xCells+1)*(m.yCells+1)*(m.zCells+1),0);
}
bool extra::blobCellVisited(const MeshIso& m,int x,int y,int z) {
    return outside(m,x,y,z) || visited[index(m,x,y,z)]!=0;
}
void extra::visitBlobCell(const MeshIso& m,int x,int y,int z) {
    if(!outside(m,x,y,z)) visited[index(m,x,y,z)]=1;
}
