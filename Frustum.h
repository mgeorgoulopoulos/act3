// Frustum.h
// Frustum clipping implementation.

// LOCK /////////////////////////////////////////

#ifndef __FRUSTUM_CLIPPING_HEADER
#define __FRUSTUM_CLIPPING_HEADER

// INCLUDES /////////////////////////////////////

#include "d3dx9.h"
#include "fvf.h"
#include "log.h"

// STRUCTS //////////////////////////////////////

struct CULLINFO
{
    D3DXVECTOR3 vecFrustum[8];    // corners of the view frustum
    D3DXPLANE planeFrustum[6];    // planes of the view frustum
};

// PROTOTYPES ///////////////////////////////////

VOID UpdateCullInfo( CULLINFO* pCullInfo, 
					D3DXMATRIXA16* pMatView, 
					D3DXMATRIXA16* pMatProj );

bool ObjectInFrustum( CULLINFO* pCullInfo, 
					 D3DXVECTOR3* pVecBounds, 
                     D3DXPLANE* pPlaneBounds);

// LOCK /////////////////////////////////////////

#endif