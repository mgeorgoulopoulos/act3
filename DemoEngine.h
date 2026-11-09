// DemoEngine.h
// (c) Michael Georgoulopoulos 2003
// 
// Description : This is a DirectX Wrapper Class used to simplify
// setting up and common tasks of DirectX9 , for Demos

// LOCK ////////////////////////////////////////////////////////////

#ifndef __DEMO_ENGINE_HEADER
#define __DEMO_ENGINE_HEADER

// INCLUDES ////////////////////////////////////////////////////////

#include <windows.h>
#include "d3dx9.h"
#define  DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "log.h"
#include "dshow.h"
#include "fvf.h"
#include <stdio.h>
#include <math.h>
#include "model.h"
#include <winbase.h>
#include "settings.h"
#include "TextureManager.h"
#include "Frustum.h"
// DEBUG
#include "Debug.h"

// CLASSES /////////////////////////////////////////////////////////
class DemoEngine;

class Scene 
{

public:

	IDirect3DDevice9*	m_pd3dDevice;
	char*				m_pName;

	DWORD				m_dwStartTime;
	DWORD				m_dwTime;
	DWORD				m_dwEndTime;
	DWORD				m_dwDuration;

	Scene*				m_pNextScene;
	DemoEngine*			m_pHostDemo;

	bool				m_bFinished;
	bool				m_bInitialized;

	Scene();
	~Scene();

	void SetName(char* name);
	void SetDuration(DWORD duration);
	void GetStartTime();
	void GetEndTime();
	void UpdateTime();

	void SceneLoop();

	bool virtual Init() {return true;};
	void virtual Render() {};
	void virtual Release() {};

} ; // End class CScene

class DemoEngine
{

public:
	// ****** WINDOWS AND DIRECTX STUFF ******\\

	// Windows
	HWND						m_hAppWindow;
	HINSTANCE					m_hAppInstance;

	// Direct3D
	IDirect3D9*					m_pD3D ;
	IDirect3DDevice9*			m_pd3dDevice;

	// DirectShow
	IGraphBuilder*				m_pGraph;
	IMediaControl*				m_pMediaControl;
	IMediaEvent*				m_pEvent;
	bool						MusicPlaying;
	LPCWSTR						m_pSong;

	// DirectInput
	LPDIRECTINPUT8				m_pDirectInput;
	LPDIRECTINPUTDEVICE8		m_pMouseDevice;
	DIMOUSESTATE				m_MouseState;
	long						m_MouseX , m_MouseY;
	bool						m_bMouseL , m_bMouseR;
	float						m_fMouseXAxis , m_fMouseYAxis;
	LPDIRECTINPUTDEVICE8		m_pKeyboardDevice;
	bool						m_bKeys[256];

	// ****** PURE DEMOENGINE STUFF ****** \\

	// Init Success
	bool		m_bSoundOK;
	bool		m_bGraphicsOK;
	bool		m_bInputOK;

	// Time related
	DWORD		m_dwTime;				// Time in ms
										// since the app started
	DWORD		m_dwStartTime;			// Starting time in ms
	DWORD		m_dwEndTime;
	bool		m_bHighPrecisionTimer;	// Is there a high-precision timer?
	LONGLONG	m_llTicksPerSec;		// Ticks per second (only for high precision timers)
	FLOAT		m_fFPS;
	DWORD		m_dwFrames;

	// Scene management
	Scene*				m_pFirstScene;
	Scene*				m_pCurrentScene;
	DWORD				m_dwNrOfScenes;

public:
	bool		InitGraphics();
	bool		InitShaders();
	void		ReleaseGraphics();
	bool		InitMusic();
	void		StartMusic();
	void		StopMusic();
	void		ReleaseMusic();
	void		SetSong(LPCWSTR song);
	bool		DemoLoop();

	// Init Input - Init this only in development of the demo
	bool		InitInput();
	void		InputLoop();
	void		ReleaseInput();


	void GetStartTime(void);			// Get the start time
	void UpdateTime(void);				// Updates m_dwTime
	bool Initialize(HINSTANCE hInst);	// Demo Init
	void Release();

	void AddScene(Scene* scene);

	// Constructor
	DemoEngine();


};// End Class DemoEngine



#include "camera.h"



// FUNCTIONS ////////////////////////////////////////////////////

inline DWORD FtoDW(float f) {return *((DWORD*)&f);}
int RenderToTexture(PDIRECT3DTEXTURE9 pTexture , PDIRECT3DDEVICE9 pDevice);
int EndRenderToTexture(PDIRECT3DDEVICE9 pDevice);
unsigned long GetColorAtPosition(PDIRECT3DDEVICE9 pDevice , float x, float y , float z);
unsigned long FlareIntensity(PDIRECT3DDEVICE9 pDevice ,float x , float y , float z);
void RenderQuadAt(PDIRECT3DDEVICE9 pDevice , 
				  int x , int y , 
				  float size,
				  unsigned long color);
void RenderSpecificQuad(PDIRECT3DDEVICE9 pDevice , 
				  float x , float y , 
				  float width ,float height,
				  unsigned long color);
void RenderGaussianQuad(PDIRECT3DDEVICE9 pDevice , 
				  float blur);
void GetXY(PDIRECT3DDEVICE9 pDevice ,
		   float x , float y , float z,
		   int* p_x , int* p_y);
void GetScreenCenter(int* p_x , int* p_y);

bool GetTransition();
void SetTransition(bool t);

// LOCK /////////////////////////////////////////////////////////

#endif