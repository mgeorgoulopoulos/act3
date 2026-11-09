// 3ds.h
// This file contains declarations useful in reading
// 3ds files

// LOCK /////////////////////////////////////////////////////

#ifndef __3DS_HEADER
#define __3DS_HEADER

// INCLUDES /////////////////////////////////////////////////

#include <stdio.h>
#include "fvf.h"
#include <malloc.h>

// DEFINES //////////////////////////////////////////////////

#define CHUNK_MAIN				0x4D4D
#define CHUNK_VERSION			0x0002
#define CHUNK_3DEDITOR			0x3D3D
#define CHUNK_OBJECTBLOCK		0x4000
#define CHUNK_TRIANGULARMESH	0x4100
#define CHUNK_VERTICESLIST		0x4110
#define CHUNK_FACESDESCRIPTION	0x4120
#define CHUNK_MATERIALLIST		0x4130
#define CHUNK_MATERIALBLOCK		0xAFFF
#define CHUNK_MATERIALNAME		0xA000
#define CHUNK_MAPPINGCOORDS		0x4140
#define CHUNK_TEXTUREBLOCK		0xA200
#define CHUNK_TEXTUREFILENAME	0xA300

// Color Chunks

// No gamma correction
#define CHUNK_COLOR_FLOAT		0x0010
#define CHUNK_COLOR_BYTE		0x0011

// Gamma corrected
#define CHUNK_COLOR_GBYTE		0x0013
#define CHUNK_COLOR_GFLOAT		0x0014

// Materials
#define CHUNK_MATERIALAMBIENT	0xA010
#define CHUNK_MATERIALDIFFUSE	0xA020
#define CHUNK_MATERIALSPECULAR	0xA030
#define CHUNK_MATERIALPOWER		0xA041

// Percent Chunks
#define CHUNK_PERCENT_INT		0x0030
#define CHUNK_PERCENT_FLOAT		0x0031

// FUNCTIONS ////////////////////////////////////////////////

int ExportObject(char* filename , FILE* f);
int GoToChunk(unsigned short c , FILE* f);
int SkipBytes(unsigned long num_of_bytes , FILE* f);
int ReadColorChunk(float* r , float* g , float* b , FILE* f);
int ReadPercentChunk(float* p , FILE* f);
int SearchForChunkUntil(unsigned short c , unsigned short until, FILE*f);

// UNLOCK ///////////////////////////////////////////////////

#endif