// Debug.h
// Definitions for writing 2d text in the back buffer
// of a d3d application . (For debug purposes)

// LOCK ////////////////////////////////////////////////////

#ifndef __SAMURAI_DEBUGTEXT_HEADER
#define __SAMURAI_DEBUGTEXT_HEADER

// INCLUDES ////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include "log.h"
#include <d3dx9.h>

// PROTOTYPES //////////////////////////////////////////////

int MakeFont(LPDIRECT3DDEVICE9 pDevice);
void ReleaseFont();
void PutText(LPDIRECT3DDEVICE9 pDevice,long x , long y,char* text);

// LOCK ////////////////////////////////////////////////////

#endif