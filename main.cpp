// main.cpp
// This is the entry point (Winmain) for the demo application

// DEFINES ////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN

// INCLUDES ////////////////////////////////////////////////////////////

#include <windowsx.h>
#include <stdio.h>
#include "Dialog.h"
#include "scenes.h" //#include "DemoEngine.h"

// GLOBALS ////////////////////////////////////////////////////////////

Scene1 s1;
Scene2 s2;
Scene3 s3;
Scene4 s4;
SpaceScene space_scene;

char text_buffer[1000];

extern HWND hIntroWnd;

TextureManager TexMan;

// WINMAIN //////////////////////////////////////////////////////////

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{

	// The demo Engine
	DemoEngine demo;

	// Start with the dialog
	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1),
	      hIntroWnd, reinterpret_cast<DLGPROC>(IntroDlgProc));


	// Add scenes here

	// Space
	space_scene.SetDuration(70000);
	//demo.AddScene(&space_scene);

	// Shadow
	s4.SetDuration(56000);
	demo.AddScene(&s4);

	// Eggplants
	s1.SetDuration(40000);
	demo.AddScene(&s1);

	// Greetz
	s2.SetDuration(19300);
	demo.AddScene(&s2);

	// Credits
	s3.SetDuration(37900);
	demo.AddScene(&s3);

	// Init the demo
	if (!demo.Initialize(hInst))
	{
		WriteToLog("Failed to initialize demo");
		demo.Release();
		return 0;
	}


	MSG msg; 
	BOOL fMessage;

	while(true)
	{
		fMessage = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);

		if(fMessage)
		{
			//Process message
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Here do rendering
			if (!demo.DemoLoop())break;
		}


		if (msg.message == WM_QUIT) break;

	}


	demo.Release();

	// DEBUG
	ReleaseFont();
	
	return 0;
}
