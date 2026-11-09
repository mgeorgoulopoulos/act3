// Debug.cpp

// INCLUDES ////////////////////////////////////////////////

#include "Debug.h"

// GLOBALS /////////////////////////////////////////////////

// DEBUG
HFONT	hFont;
LPD3DXFONT pFont=NULL;

extern bool Debug;

#define FINAL_VERSION


// GLOBALS /////////////////////////////////////////////////

// DEBUG
int MakeFont(LPDIRECT3DDEVICE9 pDevice)
{
	hFont = CreateFont(30, 0, 0, 0,
                           FW_BOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
						 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
						 DEFAULT_PITCH,
						"Comic Sans MS");

	if (!hFont) return 0;

	if (FAILED(D3DXCreateFont(pDevice,
					hFont,
					&pFont)))
	{
		WriteToLog("Failed to init font");
		return 0;
	}

	return 1;
}

// DEBUG
void ReleaseFont()
{

	DeleteObject(hFont);

}


void PutText(LPDIRECT3DDEVICE9 pDevice,
			 long x , long y,
			 char* text)
{

	if (!Debug) 
	{
		return;
	}

	RECT r;
	r.left = x;
	r.top = y;
	r.right = 10000;
	r.bottom = 10000;

	pFont->Begin();
#ifndef FINAL_VERSION
	if (FAILED(pFont->DrawText(text,
			-1,
			&r,
			DT_LEFT,
			0xffffffff)))
	{
		WriteToLog("failed to draw text");
	}
#else
	if (FAILED(pFont->DrawText(text,
			-1,
			&r,
			DT_LEFT,
			0)))
	{
		WriteToLog("failed to draw text");
	}	
#endif
			
			


	pFont->End();

}