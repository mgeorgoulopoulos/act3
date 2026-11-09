// marching.h
// Implementation of the marching cubes algorithm

#ifndef __MARCHING_CUBES
#define __MARCHING_CUBES

#include "fvf.h"

typedef struct {
   MODEL_VERTEX p[3];
} TRIANGLE;

typedef struct {
   SIMPLE_VERTEX p[8];
   double val[8];
} GRIDCELL;

// PROTOTYPES ///////////////////////////////////////

int Polygonise(GRIDCELL grid,double isolevel,TRIANGLE *triangles);

#endif