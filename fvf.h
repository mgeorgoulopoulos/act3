

#ifndef __SAMURAI_FVF_HEADER
#define __SAMURAI_FVF_HEADER

#include "d3dx9.h"

struct TRANSFORMED_VERTEX
{
    FLOAT x, y, z, rhw; // The transformed position for the vertex.
    DWORD Diffuse;        // The vertex color.
	float tu,tv;
};

#define D3DFVF_TRANSFORMED_VERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct PARTICLE_VERTEX
{
    FLOAT x, y, z; // The transformed position for the vertex.
	float point_size;
    DWORD color;        // The vertex color.
};
#define D3DFVF_PARTICLE_VERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE | D3DFVF_PSIZE)


struct COMMON_VERTEX
{
    FLOAT	x, y, z; // The model position for the vertex.
	FLOAT	nx,ny,nz;
	DWORD   diffuse;
	FLOAT   tu,tv;


};
#define D3DFVF_COMMON_VERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct MODEL_VERTEX
{
	
	FLOAT	x,y,z;
	FLOAT	nx,ny,nz;
	FLOAT	tu,tv;

};
#define FVF_MODEL (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

struct GRID_VERTEX
{

	FLOAT	x,y,z;
	DWORD	diffuse;
	FLOAT	tu,tv;

};
#define FVF_GRID (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct SIMPLE_VERTEX
{
	float x,y,z;
};

struct BILLBOARD_VERTEX
{
	
	FLOAT	x,y,z;
	DWORD	diffuse;
	FLOAT	tu,tv;

};
#define FVF_BILLBOARD (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)


struct CUSTOMVERTEX
{
    FLOAT	x, y, z , rhw; // The model position for the vertex.
	DWORD   color;
	FLOAT   tu,tv;


};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct FOURTEXTUREVERTEX
{
    FLOAT	x, y, z , rhw; // The model position for the vertex.
	DWORD   color;
	FLOAT   tu1,tv1;
	FLOAT   tu2,tv2;
	FLOAT   tu3,tv3;
	FLOAT   tu4,tv4;
};
#define D3DFVF_FOURTEXTUREVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) | D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE2(3))


#endif