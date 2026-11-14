// model.h
// reverse-engineered version of the model.h file
// lost in a blackout!
// Let's hope to make it the same again!

// LOCK /////////////////////////////////////////////

#ifndef __SAMURAI_MODEL_HEADER
#define __SAMURAI_MODEL_HEADER

// INCLUDES /////////////////////////////////////////

#include <D3DX9.h>
#include <math.h>
#include <stdio.h>
#include "log.h"
#include "3ds.h"
#include "marching.h"
#include "fvf.h"
#include "Frustum.h"

// CLASSES //////////////////////////////////////////

enum TRANSFORM_TYPE
{
	TRANSFORM_TRANSLATE,
	TRANSFORM_ROTATE,
	TRANSFORM_SCALE
}; // End enum TRANSFORM_TYPE

class NeighborSurfaces
{
public:
	unsigned short		Number;
	unsigned short*		Buffer;

	// Methods
	void Create(unsigned short n);
	void Release();
	void SetSurface(unsigned short n , unsigned short surface);

}; // End Class NeighborSurfaces


class Material
{

public:

	D3DMATERIAL9	Data;
	char			Name[40];
	char			Texture[40];
	bool			HasTexture;

	// Methods
	int		LoadFrom3DS(char* filename , char* material_name);
	void	Set(LPDIRECT3DDEVICE9 pDevice,IDirect3DBaseTexture9* Tex);
	bool	IsTheName(char* n);

}; // End Class Material

class MetaBall;

class Mesh3DS
{

public:

	// GEOMETRY MEMORY
	MODEL_VERTEX*		VertexList;
	unsigned short*		IndexList;
	unsigned short		NumVertices , NumIndices;

	// D3D BUFFERS
	IDirect3DVertexBuffer9*		pVB;
	IDirect3DIndexBuffer9*		pIB;

	// OPTIONAL GEOMETRY MEMORY
	NeighborSurfaces*	pNeighbors;
	D3DXVECTOR3*		pFaceNormals;

	// 3ds stuff
	char				Name[40];
	char				MaterialName[40];
	
	// Bounding Box
	D3DXVECTOR3			BoundingBox[8];
	D3DXVECTOR3			BoundingBoxScratch[8];
	D3DXPLANE			BBPlanes[6];
	unsigned short		BBIndices[36];

	// Transformation matrix
	D3DXMATRIXA16		Transform;

	// User defined texture?
	IDirect3DBaseTexture9*	ExternalTexture;

	// Cylinder primitive
	int					CylinderSlices;
	int					CylinderCircleSlices;

	// Constructor
	Mesh3DS();

	// Methods
	int		LoadFrom3DS(char* filename , char* object_name);
	int		CreateBuffers(PDIRECT3DDEVICE9 pDevice);
	int		RefreshBuffers();
	int		RefreshVertexBuffer();
	int		RefreshIndexBuffer();
	int		Render(PDIRECT3DDEVICE9 pDevice);
	int		BatchRender(PDIRECT3DDEVICE9 pDevice);
	void	Release();
	int		CalculateFaceNormals(bool Norm);
	int		FindNeighbors();
	int		CalculateNormals(bool Norm);
	int		AllocateSpace();
	int		CreateFrom(Mesh3DS* m);
	int		LinearInterpolate(Mesh3DS* m1 , Mesh3DS* m2 , float t);
	int		SphericalInterpolate(Mesh3DS* m1 , Mesh3DS* m2 , float t);
	bool	IsTheName(char* n);
	void	ComputeBoundingBox();
	bool	RenderClipped(LPDIRECT3DDEVICE9 pDevice , CULLINFO* frustum);
	bool	BatchRenderClipped(LPDIRECT3DDEVICE9 pDevice , CULLINFO* frustum);
	void	CalculateBBPlanes();
	void	SetBBIndices();
	void	ClearTransform();
	void	SetTransform(TRANSFORM_TYPE t , float par1,float par2,float par3);
	void	TransformBoundingBox();
	int		CalculateNormalsToVB();
	int		CalculateFaceNormalsFromVB();
	void	InvertNormals();
	void	Twist(float a , float t);

	// Geometric primitives
	bool	CreateCylinder(float Length , float Radius,int Slices,int CircleSlices);
	void	Distortion(unsigned long t);

}; // End Class Mesh3DS



class MeshGrid
{

public:

	// GEOMETRY MEMORY
	GRID_VERTEX*		VertexList;
	unsigned short*		IndexList;
	unsigned short		NumVertices , NumIndices;
	float*				Distances;

	// D3D BUFFERS
	IDirect3DVertexBuffer9*		pVB;
	IDirect3DIndexBuffer9*		pIB;

	// Transformation matrix
	D3DXMATRIXA16		Transform;

	// Methods
	int		CreateBuffers(PDIRECT3DDEVICE9 pDevice);
	int		RefreshBuffers();
	int		Render(PDIRECT3DDEVICE9 pDevice);
	void	Release();
	int		ApplyColor(unsigned long c);
	int		CreateRectangle(unsigned short m , unsigned short n , 
							float width , float height);
	int		VortexRectangle(float a);
	int		Distort(float a);
	int	    Graviball(float x , float y , float a);
	int		RadialFill(float r);
	void	Scroll(float x, float y);
	void	ClearTransform();
	void	SetTransform(TRANSFORM_TYPE t , float par1,float par2,float par3);

}; // End Class MeshGrid


class MetaBall
{
public:

	float	x,y,z;
	float	mass;

}; // End class MetaBall

class MeshIso
{

public:

	// GEOMETRY MEMORY
	MODEL_VERTEX*		VertexList;
	unsigned short		NumVertices , NumCells , NumTriangles;

	double				IsoLevel;
	float				GridWidth,GridHeight,GridDepth;
	int					xCells , yCells , zCells;
	unsigned short*		ActiveCells;
	unsigned short		NumActiveCells;

	// D3D BUFFERS
	IDirect3DVertexBuffer9*		pVB;
	IDirect3DIndexBuffer9*		pIB;

	// Metaballs
	MetaBall*					pBalls;
	unsigned short				NumBalls;

	// Methods

	int		CreateBuffers(PDIRECT3DDEVICE9 pDevice);
	int		RefreshBuffers();
	int		Render(PDIRECT3DDEVICE9 pDevice);
	void	Release();
	int		Create(unsigned short x_cells ,
				   unsigned short y_cells ,
				   unsigned short z_cells ,
				   float width,
				   float height,
				   float depth,
				   int NumBalls);
	void	Update();
	int		CalculateCell(int x , int y , int z);
	bool	IsCellCalculated(int x , int y , int z);
	void	AddCellToCalculated(int x , int y , int z);
	bool	CalculateNeighbors(int x,int y,int z);

}; // End Class MeshIso




class MeshInCube
{

public:

	// GEOMETRY MEMORY
	GRID_VERTEX*		VertexList;
	unsigned short*		IndexList;
	unsigned short		NumVertices , NumIndices;

	// D3D BUFFERS
	IDirect3DVertexBuffer9*		pVB;
	IDirect3DIndexBuffer9*		pIB;

	// Methods
	int		CreateBuffers(PDIRECT3DDEVICE9 pDevice);
	int		RefreshBuffers();
	int		Render(PDIRECT3DDEVICE9 pDevice);
	void	Release();
	int		ApplyColor(unsigned long c);
	int		Create(float size);

}; // End Class MeshInCube

class MaterialManager
{
public:

	Material*		MaterialList;
	unsigned long	NumMaterials;

	int				LoadFrom3DS(char* filename);
	void			Release();
	Material*		GetMaterial(char* n);
	void			SetMaterial(LPDIRECT3DDEVICE9 pDevice , char* n,
										IDirect3DBaseTexture9* Tex);

}; // End class MaterialManager

class Scene3DS
{
public:
	Mesh3DS*		ObjectList;
	unsigned long	NumObjects;

	MaterialManager	MaMa;

	D3DXMATRIXA16	Transform;

	// Constructor
	Scene3DS();

	int				LoadFrom3DS(LPDIRECT3DDEVICE9 pDevice,
									char* filename);
	Mesh3DS*		GetObject(char* n);
	void			RenderObject(LPDIRECT3DDEVICE9 pDevice , char* n);
	void			Render(LPDIRECT3DDEVICE9 pDevice);
	void			Release();
	unsigned long	RenderClipped(LPDIRECT3DDEVICE9 pDevice,
							 CULLINFO* frustum);
	void			BatchRender(LPDIRECT3DDEVICE9 pDevice);
	unsigned long	BatchRenderClipped(LPDIRECT3DDEVICE9 pDevice,
							 CULLINFO* frustum);

	// S3D file type
	void			SaveToS3D(char* filename);
	int				LoadFromS3D(char* filename , LPDIRECT3DDEVICE9 pDevice);

	// Transform
	void	ClearTransform();
	void	SetTransform(TRANSFORM_TYPE t , float par1,float par2,float par3);

}; // End class Scene3DS


/////////////////////////////////////////////////////

class Particle
{
public:
	float	x,y,z;
	DWORD	color;
	float	size;
	bool	alive;

	// constructor
	Particle();

}; // End class Particle

class MeshBillboard
{
public:

	// Particles
	Particle			*ParticleList;
	unsigned long		NumParticles;

	// GEOMETRY MEMORY
	BILLBOARD_VERTEX*	VertexList;

	// D3D BUFFERS
	IDirect3DVertexBuffer9*		pVB;

	// Transformation matrix
	D3DXMATRIXA16		Transform;

	// Constructor
	MeshBillboard();

	// Methods
	int		Init(PDIRECT3DDEVICE9 pDevice , unsigned long num_particles);
	int		CreateBuffers(PDIRECT3DDEVICE9 pDevice);
	int		RefreshVertexBuffer();
	int		Render(PDIRECT3DDEVICE9 pDevice);
	void	Release();
	void	ClearTransform();
	void	SetTransform(TRANSFORM_TYPE t , float par1,float par2,float par3);
	unsigned long NumActiveParticles();
	void	CreateFrom(PDIRECT3DDEVICE9 pDevice , Mesh3DS* pMesh);
	void	UpdateFrom(Mesh3DS* pMesh);

}; // End class MeshBillBoard

// LOCK /////////////////////////////////////////////

#endif