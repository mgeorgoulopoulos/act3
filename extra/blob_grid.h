#pragma once
class MeshIso;
namespace extra {
int blobResolution();
void beginBlobGrid(const MeshIso& mesh);
bool blobCellVisited(const MeshIso& mesh,int x,int y,int z);
void visitBlobCell(const MeshIso& mesh,int x,int y,int z);
}
