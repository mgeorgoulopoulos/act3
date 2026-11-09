// DemoEngine.cpp
// (c) Michael Georgoulopoulos 2003

// INCLUDES ///////////////////////////////////////////////////////

#include "DemoEngine.h"
#include "resource.h"

// GLOBALS ////////////////////////////////////////////////////////

LPDIRECT3DSURFACE9	pRenderSurface=NULL , pBackBuffer=NULL;

DWORD last_fps_count=1500;
DWORD frames = 0;

extern bool Music,Windowed,Input,Begin,Debug;
extern unsigned long Width , Height;
unsigned long RenderTargetWidth , RenderTargetHeight;

extern TextureManager TexMan;

bool Transition = false;
bool BlurEnable = true;

LPD3DXBUFFER pShader;
LPD3DXBUFFER pErrorMsgs;
LPDIRECT3DPIXELSHADER9 pBlurShader;

// winproc //////////////////////////////////////////////////////////

//The windows message handler
LRESULT WINAPI WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
HDC hdc;
PAINTSTRUCT ps;

    switch(msg)
    {
		case WM_PAINT:    //Repaint our window
			 hdc=BeginPaint(hWnd,&ps);
			 EndPaint(hWnd,&ps);
			 return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        break;
        case WM_KEYUP: 
            switch (wParam)
            { 
                case VK_ESCAPE:
                    //User has pressed the escape key, so quit
                    DestroyWindow(hWnd);
                    return 0;
                break;
            } 
        break;

    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// FUNCTIONS //////////////////////////////////////////////////////

DemoEngine::DemoEngine()
{

	m_pD3D					=NULL;
	m_pd3dDevice			=NULL;

	m_pGraph				=NULL;
	m_pMediaControl			=NULL;
	m_pEvent				=NULL;
	MusicPlaying			=false;
	m_pSong					=NULL;

	m_dwTime				=0;	
	m_dwFrames				=0;
									
	m_dwStartTime			=0;		
	m_dwEndTime				=0;

	m_pFirstScene			=NULL;
	m_pCurrentScene			=NULL;
	m_dwNrOfScenes			=0;

	m_MouseX = 400;
	m_MouseY = 300;

	m_bSoundOK = false;
	m_bGraphicsOK = false;
	m_bInputOK = false;
}

bool DemoEngine::Initialize(HINSTANCE hInst) 
{
	if (!Begin) return 0;

	// Open the log file
	OpenLog();

	// Get global Access to the application's Instance
	m_hAppInstance = hInst;

	// First Initialize Graphics
	if (!InitGraphics())
	{
		WriteToLog("Failed to init graphics - shutting down");
		return 0;
	}
	m_bGraphicsOK = true;
	// Init Shaders
	InitShaders();

	// While other things are loading , display the loading message
	MeshGrid m1;
	m1.CreateRectangle(2,2,10,6);
	m1.CreateBuffers(m_pd3dDevice);
	m1.ApplyColor(0xffffffff);
	m1.RefreshBuffers();

	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);

	// set the "loading" texture
	TexMan.SetTexture(m_pd3dDevice , 0,"loading.png");

	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 4.0f/3.0f, 1.0f, 10000.0f );
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);

	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_TRANSLATE , 0,0,11);
	m1.Render(m_pd3dDevice);

	m_pd3dDevice->Present(NULL , NULL , NULL , NULL);


	// Set the song
	if (Music) SetSong(SONG_NAME);

	// Initialize the music
	
	if (Music)
	{
		if (!InitMusic())
		{
			WriteToLog("Failed to init music - shutting down");
		}
		else
		{
			m_bSoundOK = true;
		}
	}
	m_bSoundOK = true;


	Scene* scene = m_pFirstScene;

	if (!scene) 
	{
		WriteToLog("No scenes available . Shutting down...");
		return 0;
	}

	int i=1;
	char text_buffer[1000];
	while (scene) {
	
		sprintf(text_buffer , "Initializing scene: <b>%d</b>" , i);
		WriteToLog(text_buffer);
		if (!scene->Init())
		{
			WriteToLog("Unable to initialize some scene(s)");
			return 0;
		}
		else
		{
			scene->m_bInitialized = true;
		}
		scene = scene->m_pNextScene;
		i++;
	}



	if (Input)
	{
		//  Init Input
		if (!InitInput())
		{
			WriteToLog("Failed to init <b>input</b>");
			return 0;
		}
		m_bInputOK = true;
	}

	// release the quad for the loading screen
	m1.Release();

	// Start Playing the music
	if (Music && m_bSoundOK) StartMusic();

	// Debug output
	if (Debug)
	{
		MakeFont(m_pd3dDevice);
	}

	// Do this last
	GetStartTime();

	// Return success
	return true;

} // End Initialize()

///////////////////////////////////////////////////////////////

void DemoEngine::GetStartTime(void) 
{

	LARGE_INTEGER qwTicksPerSec;
	static LARGE_INTEGER Ticks;

	if (!QueryPerformanceFrequency(&qwTicksPerSec))
	{
		m_bHighPrecisionTimer = false;
		m_dwStartTime = GetTickCount();
		WriteToLog("No <b>High - precision Timer</b> found - switching to GetTickCount()");
	}
	else
	{
		m_bHighPrecisionTimer = true;
		m_llTicksPerSec = qwTicksPerSec.QuadPart;
		QueryPerformanceCounter(&Ticks);
		m_dwStartTime = (unsigned long)((Ticks.QuadPart*1000) / m_llTicksPerSec);
		WriteToLog("<b>High - precision Timer</b> found!");
	}



} // End GetStartTime()

//////////////////////////////////////////////////////////////

void DemoEngine::UpdateTime() {

	LARGE_INTEGER Ticks;
	DWORD temp_time;

	if (m_bHighPrecisionTimer)
	{
		QueryPerformanceCounter(&Ticks);
		temp_time = (unsigned long)((Ticks.QuadPart*1000) / m_llTicksPerSec) - m_dwStartTime;
	}
	else
	{
		temp_time = GetTickCount() - m_dwStartTime;
	}


	DWORD temp2 = temp_time - m_dwTime;
	m_dwTime += temp2;

	last_fps_count -= temp2;
	frames++;

	if (last_fps_count <= 500)
	{
		// Find fps
		DWORD time_passed = 1500 - last_fps_count;
		m_fFPS = (float) frames * 1000.0f / (float) (time_passed);

		last_fps_count = 1500;
		frames = 0;
	}

} // End UpdateTime()

///////////////////////////////////////////////////////////////


void DemoEngine::Release() 
{
	// Release the scenes
	Scene* scene = m_pFirstScene;

	while (scene) 
	{
		if ( (!scene->m_bFinished)&& (scene->m_bInitialized) ) scene->Release();
		scene = scene->m_pNextScene;
	}


	if (Debug)
	{
		ReleaseFont();
	}

	if (m_bSoundOK)
	{
		// Stop the music if it is still playing
		if (Music) StopMusic();

		// Release music
		if (Music) ReleaseMusic();

	}

	// Release input
	if (m_bInputOK) ReleaseInput();


	// Release the graphics
	if (m_bGraphicsOK) ReleaseGraphics();

	float fps = (float)m_dwFrames/((float)m_dwTime/1000.0f);
	char out_str[100];

	ShowCursor(1);

	sprintf(out_str , "FPS: %f",fps);

	// Report fps
	WriteToLog(out_str);

	// Close the log file
	CloseLog();

} // End Release

////////////////////////////////////////////////////////////////////

bool DemoEngine::DemoLoop() {

	static char text_buffer[256];

	// Get Input
	if (m_bInputOK) InputLoop();

	// Update time
	UpdateTime();

	// Advance frame count
	m_dwFrames++;

	if (m_pCurrentScene) 
	{
		m_pCurrentScene->UpdateTime();
		m_pCurrentScene->SceneLoop();
		if (m_pCurrentScene->m_bFinished) 
		{
			if (m_pCurrentScene->m_pNextScene) 
			{
				m_pCurrentScene = m_pCurrentScene->m_pNextScene;
				WriteToLog("Scene Changing...");
				sprintf(text_buffer , "Scene duration : %d<br>" , m_pCurrentScene->m_dwDuration);
				WriteToLog(text_buffer);
				sprintf(text_buffer , "Now time : %d ms<br>" , m_dwTime);
				WriteToLog(text_buffer);
				sprintf(text_buffer , "Scene Time : %d ms<br><hr>" , m_pCurrentScene->m_dwTime);
				WriteToLog(text_buffer);
				
				// Start the internal scene clock
				m_pCurrentScene->GetStartTime();
			}
			else 
			{
				return 0;
			}
		}

	}

	//////////////////////////////// MUSIC LOOP-----------------
	if (MusicPlaying)
	{
		// See if the music is finished
		long event_code , event_parm1 , event_parm2;
		m_pEvent->GetEvent( &event_code , 
			&event_parm1,
			&event_parm2,
			1);
		if (event_code == EC_COMPLETE)
		{
			m_pMediaControl->Stop();
			MusicPlaying = false;
		}
	}


	return 1;

} // End DemoLoop()*/

////////////////////////////////////////////////////////////////////

void DemoEngine::AddScene(Scene* scene){

	Scene*		p_scene;

	// First check if there is a scene linked list
	if (m_pFirstScene) {
		p_scene = m_pFirstScene;
		while (p_scene->m_pNextScene) {
			p_scene = p_scene->m_pNextScene;
		}
		scene->m_pHostDemo = this;
		p_scene->m_pNextScene = scene;
		m_dwNrOfScenes ++;
	}
	else {
		m_pFirstScene = scene;
		scene->m_pHostDemo = this;
		m_dwNrOfScenes = 1;
		m_pCurrentScene = scene;
	}

}

////////////////////////////////////////////////////////////////////

bool DemoEngine::InitGraphics()
{

	/*
	// ********** WINDOW CREATION *********** \\
    // Register the window class.
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_OWNDC, WinProc, 0L, 0L, 
                      GetModuleHandle(NULL), NULL, LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1)), NULL, NULL,
                      "RE", NULL };

    RegisterClassEx( &wc );

    // Create the application's window.
    HWND hWnd = CreateWindow( "RE", "ReversedDesrever", 
                              WS_OVERLAPPED, 100, 100, Width, Height,
                              GetDesktopWindow(), NULL, wc.hInstance, NULL );


	if (Windowed) SetWindowLong( hWnd, GWL_STYLE, WS_SYSMENU|WS_OVERLAPPED );
	else          SetWindowLong( hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU|WS_VISIBLE );

	// Show the window
   ShowWindow(hWnd,SW_SHOW);
   UpdateWindow(hWnd);
   SetFocus(hWnd);

	// Hide the mouse cursor (if in fullscreen)i
	if (!Windowed) ShowCursor(0);
*/


unsigned long window_width, window_height;
WNDCLASS window_class;
DWORD style;
RECT client_rect;

   window_class.style          = CS_OWNDC;
   window_class.cbClsExtra     = 0;
   window_class.cbWndExtra     = 0;
   window_class.hInstance      = GetModuleHandle(NULL);
   window_class.hIcon          = LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
   window_class.hCursor        = LoadCursor(NULL,IDC_ARROW);
   window_class.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
   window_class.lpszMenuName   = NULL;
   window_class.lpszClassName  = "RE";
   window_class.lpfnWndProc    = WinProc;


   //Register the class with windows
   RegisterClass(&window_class);

   unsigned long desktop_width , desktop_height;
   desktop_width = GetSystemMetrics(SM_CXSCREEN);
   desktop_height = GetSystemMetrics(SM_CYSCREEN);

   //If we're running full screen, we cover the desktop with our window.
   //This isn't necessary, but it provides a smoother transition for the
   //user, especially when we're going to change screen modes.
   if(!Windowed){
      window_width=desktop_width;
      window_height=desktop_height;
      style=WS_POPUP;
   }else{
      //In windowed mode, we just make the window whatever size we need.

      style=WS_OVERLAPPED|WS_SYSMENU;

      window_width=Width;
      window_height=Height;

   }

   HWND hWnd;

   hWnd=CreateWindow("RE","ReversedDesrever",style,
                              (desktop_width-window_width)/2,(desktop_height - window_height)/2
							  ,window_width,window_height,NULL,NULL,
                              GetModuleHandle(NULL),NULL);

   if(!hWnd){
     return 0;
   }

   //Adjust window to account for borders & such
   if(Windowed){
      GetClientRect(hWnd,&client_rect);

      MoveWindow(hWnd,
                 (desktop_width-window_width)/2,(desktop_height - window_height)/2,
                 Width+(Width-client_rect.right),    //New Width
                 Height+(Height-client_rect.bottom), //New Height
                 FALSE);

   }

   //The next 3 lines just make sure that our window is visible and has the
   //input focus.  It's not strictly necessary, but it doesn't hurt to be
   //thorough.
   ShowWindow(hWnd,SW_SHOW);
   UpdateWindow(hWnd);
   SetFocus(hWnd);

	if (!Windowed) ShowCursor(0);

	// Get global access to the application's window
	m_hAppWindow = hWnd;

	// Create the d3d object
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (m_pD3D == NULL) return false;

	// presentation parameters
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed               = Windowed;
    d3dpp.BackBufferCount        = 1;
    d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_FLIP;
    d3dpp.EnableAutoDepthStencil = TRUE;
	//d3dpp.PresentationInterval	 = D3DPRESENT_INTERVAL_IMMEDIATE;

	// DEBUG !!!
	// *********
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  
    d3dpp.hDeviceWindow          = hWnd;
  
    d3dpp.BackBufferWidth  = Width;
    d3dpp.BackBufferHeight = Height;
    
	// Get the default adapter's caps
	D3DCAPS9 DeviceCaps;
	m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
		&DeviceCaps);

	// write the caps to a logfile
	FILE* CapsLog;
	CapsLog = fopen("caps_log.txt" , "w+b");
	
	fprintf(CapsLog , "Maximum Samplers : %d\n" , DeviceCaps.MaxSimultaneousTextures);
	
	if (DeviceCaps.MaxSimultaneousTextures < 4) BlurEnable = false;

	if (DeviceCaps.PixelShaderVersion < D3DPS_VERSION(1,1))
	{
		fprintf(CapsLog , "Pixel shaders 1.1 NOT supported\n");
		BlurEnable = false;
	}
	else
	{
		fprintf(CapsLog , "Pixel shaders 1.1 supported!\n");
	}

	fclose(CapsLog);

    // Create the device
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    if ( SUCCEEDED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                               hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
							   &d3dpp,
                               &m_pd3dDevice ) ) )
	{
		WriteToLog("32bpp and 24bit depth selected");
		return true;
	}

	d3dpp.AutoDepthStencilFormat = D3DFMT_D15S1;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	if ( SUCCEEDED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
							   hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
							   &d3dpp,
							   &m_pd3dDevice ) ) )
	{
		WriteToLog("32bpp and 16bit depth selected");
		return true;
	}

	d3dpp.AutoDepthStencilFormat = D3DFMT_D15S1;
	d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
	if ( SUCCEEDED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
							   hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
							   &d3dpp,
							   &m_pd3dDevice ) ) )
	{
		WriteToLog("16bpp and 16bit depth selected");
		return true;
	} 

    return false;

}

bool DemoEngine::InitShaders()
{
	// create a pixel shader
	if (BlurEnable)
	{
		char *shader = "ps.1.1 \
						def c0, 0.250000, 0.250000, 0.250000, 0.000000\
						tex t0\
						tex t1\
						tex t2\
						tex t3\
						mul r0.rgb, t1, c0\
						+ mul r1.a, t1.a, c0.b\
						mad t0.rgb, t0, c0, r0\
						+ mad t0.a, t0.a, c0.b, r1\
						mad t0.rgb, t2, c0, t0\
						+ mad t0.a, t2.a, c0.b, t0.a\
						mad r0.rgb, t3, c0, t0\
						+ mad r0.a, t3.a, c0.b, t0.a";
		
		/*
		char *shader = "ps.1.1 \
						def c1, 0.25f, 0.25f, 0.25f, 0.25f \
						tex t0 \
						tex t1 \
						tex t2 \
						tex t3 \
						mul r0, c1, t0 \
						mad r0, c1, t1, r0 \
						mad r0, c1, t2, r0 \
						mad r0, c1, t3, r0";

		
		
		char *shader =	"ps.1.1 \
						def c1, 0.5f, 1.0f, 0.3f, 0.0f \
						tex t0 \
						tex t1 \
						tex t2 \
						tex t3 \
						dp3 r0, t0, c1 \
						mul r0, r0_bx2, r0_bx2";

		
		
		char *shader = "ps.1.1 \
						tex t0  \
						tex t1 \
						tex t2 \
						tex t3 \
						// r0 = 4*t0 - t1 - t2 - t3 \
						// rearranged, this is r0 = t0 - t1 + t0 - t2 + t0 - t3 + t0 \
						mov     r0, t0 \
						sub     r0, r0,   t1 \
						add     r0, r0,   t0 \
						sub     r0, r0,   t2 \
						add     r0, r0,   t0 \
						sub     r0, r0,   t3 \
						add     r0, r0,   t0";

		
		char *shader = "ps.1.1 \
					def c1, 0.3333f, 0.3333f, 0.3333f, 0.0f \
					tex t0  \
					tex t1 \
					tex t2 \
					tex t3 \
					dp3 r0, t1, c1";

		*/
		
		if (FAILED(D3DXAssembleShader(shader,
			strlen(shader),
			NULL,
			NULL,
			0,
			&pShader,
			&pErrorMsgs
			)))
		{
			FILE* ShaderLog;
			ShaderLog = fopen("shader_log.txt" , "w+b");
			fprintf(ShaderLog , "Blur pixel shader could NOT be assembled\n");
			fclose(ShaderLog);
			BlurEnable = false;
			return false;
		}
		else
		{
			FILE* ShaderLog;
			ShaderLog = fopen("shader_log.txt" , "w+b");
			fprintf(ShaderLog , "Blur pixel shader assembled ALLRIGHT!\n");
			fclose(ShaderLog);
		}

		// create the pixel shader
		if (FAILED(m_pd3dDevice->CreatePixelShader( 
			(DWORD*)pShader->GetBufferPointer(), 
            &pBlurShader )))
		{
			FILE* ShaderLog;
			ShaderLog = fopen("shader_log.txt" , "a+b");
			fprintf(ShaderLog , "Blur pixel shader could NOT be created\n");
			fclose(ShaderLog);
			BlurEnable = false;
			return false;
		}
		else
		{
			FILE* ShaderLog;
			ShaderLog = fopen("shader_log.txt" , "a+b");
			fprintf(ShaderLog , "Blur pixel shader created OK!\n");
			fclose(ShaderLog);
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////

void DemoEngine::ReleaseGraphics()
{

    if( m_pd3dDevice != NULL)
        m_pd3dDevice->Release();
    if( m_pD3D != NULL)
        m_pD3D->Release();

}

////////////////////////////////////////////////////////////////

bool DemoEngine::InitMusic()
{

	// Initialize COM
	if (FAILED (CoInitialize(NULL)))
	{
		WriteToLog("Failed to initialize COM , for music playback... Shutting down the app");
		return false;
	}

	// Create a filter graph manager
	if (FAILED (CoCreateInstance(CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder,
		(void **) &m_pGraph) ))
	{
		WriteToLog("Failed to a filter graph instance, for music playback... Shutting down the app");
		return false;
	}

	// Query interface to get the media control
	if (FAILED(m_pGraph->QueryInterface(IID_IMediaControl , 
		(void **) &m_pMediaControl)))
	{
		WriteToLog("QueryInterface for media control object FAILED , shutting Down the app");
		return false;
	}

	// Query interface to get the media event interface
	if (FAILED(m_pGraph->QueryInterface(IID_IMediaEvent , 
		(void **) &m_pEvent)))
	{
		WriteToLog("QueryInterface for media event object FAILED , shutting Down the app");
		return false;
	}

	// We have them all , so read the file
	if (FAILED(m_pGraph->RenderFile(m_pSong , NULL)))
	{
		WriteToLog("Unable to load music file , shutting down ...");
		return false;
	}

	return true;

}

//////////////////////////////////////////////////////////////////

void DemoEngine::StartMusic()
{
	m_pMediaControl->Run();
}

void DemoEngine::StopMusic()
{

	m_pMediaControl->Stop();

}

//////////////////////////////////////////////////////////////////

void DemoEngine::ReleaseMusic()
{
	// Clean everything up
	if (m_pEvent != NULL)
		m_pEvent->Release();
	if (m_pMediaControl != NULL)
		m_pMediaControl->Release();
	if (m_pGraph != NULL)
		m_pGraph->Release();
}

//////////////////////////////////////////////////////////////////

void DemoEngine::SetSong(LPCWSTR song)
{

	m_pSong = song;

}

//////////////////////////////////////////////////////////////////

bool DemoEngine::InitInput()
{

	// Create the directInput Object
	if (FAILED(DirectInput8Create(m_hAppInstance, 
					DIRECTINPUT_VERSION, 
					IID_IDirectInput8, 
					(void**)&m_pDirectInput, NULL)))
	{
		WriteToLog("Failed to Create the DirectInput object");
		return false;
	}

	// Create the mouse device
	if (FAILED(m_pDirectInput->CreateDevice(GUID_SysMouse, 
						&m_pMouseDevice, 
						NULL)))
	{
		WriteToLog("Failed to Init a mouse device");
		return false;
	}

	// Set the data format for the mouse
	if (FAILED(m_pMouseDevice->SetDataFormat(&c_dfDIMouse)))
	{
		WriteToLog("Failed to set the data format of the mouse");
		return false;
	}

	if (FAILED(m_pMouseDevice->SetCooperativeLevel(m_hAppWindow,
               /*DISCL_EXCLUSIVE | DISCL_FOREGROUND*/DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		WriteToLog("Failed to set the cooperation level of the mouse");
		return false;
	}

	if (FAILED(m_pMouseDevice->Acquire()))
	{
		WriteToLog("Unable to Acquire the mouse");
		return false;
	}


	// ****** KeyBoard ******

	// Create the keyboard object
	if (FAILED(m_pDirectInput->CreateDevice(GUID_SysKeyboard, 
		&m_pKeyboardDevice,
		NULL)))
	{
		WriteToLog("Unable to create the keyboard object");
		return false;
	}

	// Set the keyboard data format
	if (FAILED(m_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard)))
	{
		WriteToLog("Failed to set the keyboard's data format");
		return false;
	}

	// Set cooperation level
	if (FAILED(m_pKeyboardDevice->SetCooperativeLevel(m_hAppWindow, 
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		WriteToLog("Failed to set coop level for the keyboard");
		return false;
	}

	// Aqcuire the keyb
	if (FAILED(m_pKeyboardDevice->Acquire()))
	{
		WriteToLog("Failed to acquire the keyboard");
		return false;
	}


	return true;

}

void DemoEngine::InputLoop()
{
	// Get the mouse status
	m_pMouseDevice-> GetDeviceState(sizeof(DIMOUSESTATE),
									&m_MouseState);

	// refresh mouse position
	m_MouseX += m_MouseState.lX;
	m_MouseY += m_MouseState.lY;

	// Refresh mouse axes
	m_fMouseXAxis = m_MouseState.lX * MOUSE_SPEED;
	m_fMouseYAxis = m_MouseState.lY * MOUSE_SPEED;


	// refresh mouse buttons
	if (m_MouseState.rgbButtons[0] & 0x80) m_bMouseL = true;
	else m_bMouseL = false;

	if (m_MouseState.rgbButtons[1] & 0x80) m_bMouseR = true;
	else m_bMouseR = false;

	// Center the mouse if button 3 is pressed
	if (m_MouseState.rgbButtons[2] & 0x80) 
	{
		m_MouseX = 0;
		m_MouseY = 0;
	}

	// ****** KEYBOARD ******

	// Refresh the buttons
	static char buttons[256];
	m_pKeyboardDevice->GetDeviceState(sizeof(buttons),(LPVOID)buttons); 

	// Convert the chars into bools
	for (int key = 0;key<256;key++)
	{
		if (buttons[key] & 0x80)
		{
			m_bKeys[key] = true;
		}
		else
		{
			m_bKeys[key] = false;
		}
	}

}

void DemoEngine::ReleaseInput()
{

	// Unacquire the keyboard
	if (m_pKeyboardDevice)
		m_pKeyboardDevice->Unacquire();

	// Release the mouse device
	if (m_pKeyboardDevice)
		m_pKeyboardDevice->Release();
	
	// Unacquire the mouse
	if (m_pMouseDevice)
		m_pMouseDevice->Unacquire();

	// Release the mouse device
	if (m_pMouseDevice)
		m_pMouseDevice->Release();

	// Release the directInput object
	if (m_pDirectInput)
		m_pDirectInput->Release();

}

////////////////////////////////////////////////////////////////////

// ****** Scene Functions ****** \\

Scene::Scene()
{

	m_dwStartTime = 0;
	m_dwEndTime = 0;
	m_dwTime = 0;
	m_dwDuration = 0;

	m_pNextScene = NULL;

	m_bInitialized = false;

}

////////////////////////////////////////////////

Scene::~Scene(){


}

////////////////////////////////////////////////

void Scene::GetStartTime() 
{

	m_dwStartTime = m_pHostDemo->m_dwTime;

}

////////////////////////////////////////////////

void Scene::GetEndTime() {
	m_dwEndTime = m_pHostDemo->m_dwTime;
}

////////////////////////////////////////////////

void Scene::UpdateTime() 
{

	m_dwTime = m_pHostDemo->m_dwTime - m_dwStartTime;

}

////////////////////////////////////////////////////////

void Scene::SceneLoop() {

	UpdateTime();

	if (m_dwTime > m_dwDuration) m_bFinished = true;

	Render();

}

////////////////////////////////////////////////

void Scene::SetDuration(DWORD duration) {

	m_dwDuration = duration;

}

/////////////////////////////////////////////////

void Scene::SetName(char* name) {

	m_pName = name;

}

// RENDER-TO-TEXTURE Utility ///////////////////

int RenderToTexture(PDIRECT3DTEXTURE9 pTexture , PDIRECT3DDEVICE9 pDevice)
{
	pDevice->SetTexture(0,NULL);

	// Get the texture's surface
	if (FAILED(pTexture->GetSurfaceLevel(0,&pRenderSurface)))
	{
		WriteToLog("Failed to get the texture's surface");
		return 0;
	}

	RenderTargetWidth = Width;
	RenderTargetHeight = Height;
	Width = 512;
	Height = 512;

	// Get the back Buffer surface
	if (FAILED(pDevice->GetRenderTarget(0,&pBackBuffer)))
	{
		WriteToLog("Failed to get the BackBuffer's surface");
		return 0;
	}


	// Start rendering to the texture
	if (FAILED(pDevice->SetRenderTarget(0,pRenderSurface)))
	{
		WriteToLog("Failed to get the render target surface");
		return 0;
	}

	// Cleanup
	pRenderSurface->Release();
	pRenderSurface = NULL;	

	return 1;
}

int EndRenderToTexture(PDIRECT3DDEVICE9 pDevice)
{

	//render scene with texture
	pDevice->SetRenderTarget(0,pBackBuffer);	

	Width = RenderTargetWidth;
	Height = RenderTargetHeight;

	pBackBuffer->Release();
	pBackBuffer = NULL;

	return 1;

}

// Get color utility /////////////////

unsigned long GetColorAtPosition(PDIRECT3DDEVICE9 pDevice , float x, float y , float z)
{
	//Create a vector
	D3DXVECTOR3 vec;
	vec.x = x;
	vec.y = y;
	vec.z = z;

	// get all the transformations
	D3DXMATRIXA16 world , view , projection ,viewport;
	pDevice->GetTransform(D3DTS_WORLD , &world);
	pDevice->GetTransform(D3DTS_VIEW , &view);
	pDevice->GetTransform(D3DTS_PROJECTION , &projection);

	viewport = D3DXMATRIXA16(
		(float)Width/2 ,	0,					0,			0,
		0,					-(float)Height/2,	0,			0,
		0,					0,					1,			0,
		(float)Width/2	,	(float)Height/2,	0,			1
	);

	// multiply the 3 matrices
	D3DXMATRIXA16 transform;
	D3DXMatrixMultiply(&transform , &world , &view);
	D3DXMatrixMultiply(&transform , &transform , &projection);
	D3DXMatrixMultiply(&transform , &transform , &viewport);

	// Tranform the vector to get the screen coords
	D3DXVECTOR4 vec_result;
	D3DXVec3Transform(&vec_result , &vec , &transform);


	vec.x = vec_result.x / vec_result.w;
	vec.y = vec_result.y / vec_result.w;


	// see if we are in the screen
	if (vec.x<0 || vec.x>Width) return 0;
	if (vec.y<0 || vec.y>Height) return 0;


	int ix = (int)vec.x;
	int iy = (int)vec.y;

	// Get The Back Buffer
	IDirect3DSurface9 *pBackBuffer;
	pDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,
		&pBackBuffer);

	// Lock it
	D3DLOCKED_RECT LockedRect;
	pBackBuffer->LockRect(&LockedRect , NULL , 0);
	DWORD* pPixels = (DWORD*) LockedRect.pBits;
	int pitch = LockedRect.Pitch / 4;

		unsigned long pix = pPixels[ix+iy*pitch];

	// Unlock
	pBackBuffer->UnlockRect();

	return pix;
}

unsigned long FlareIntensity(PDIRECT3DDEVICE9 pDevice ,float x , float y , float z)
{
	//Create a vector
	D3DXVECTOR3 vec;
	vec.x = x;
	vec.y = y;
	vec.z = z;

	// get all the transformations
	D3DXMATRIXA16 world , view , projection ,viewport;
	pDevice->GetTransform(D3DTS_WORLD , &world);
	pDevice->GetTransform(D3DTS_VIEW , &view);
	pDevice->GetTransform(D3DTS_PROJECTION , &projection);

	viewport = D3DXMATRIXA16(
		(float)Width/2 ,	0,					0,			0,
		0,					-(float)Height/2,	0,			0,
		0,					0,					1,			0,
		(float)Width/2	,	(float)Height/2,	0,			1
	);

	// multiply the 3 matrices
	D3DXMATRIXA16 transform;
	D3DXMatrixMultiply(&transform , &world , &view);
	D3DXMatrixMultiply(&transform , &transform , &projection);
	D3DXMatrixMultiply(&transform , &transform , &viewport);

	// Tranform the vector to get the screen coords
	D3DXVECTOR4 vec_result;
	D3DXVec3Transform(&vec_result , &vec , &transform);


	vec.x = vec_result.x / vec_result.w;
	vec.y = vec_result.y / vec_result.w;


	// see if we are in the screen
	if (vec.x<0 || vec.x>Width) return 0;
	if (vec.y<0 || vec.y>Height) return 0;

	int ix = (int)vec.x;
	int iy = (int)vec.y;

	// 10 percent of the screen;s width/height will be the border
	int borderX = Width / 10;
	int borderY = Height/10;

	if (ix > borderX && ix<(int)Width-borderX && iy> borderY && iy < (int)Height-borderY)
		// full intensity
		return 0xffffffff;

	float c;

	if (ix < borderX)			c = (float)ix / (float) borderX;
	if (ix > (int)Width - borderX)	c = (float)(borderX - (ix-(Width-borderX))) / (float) borderX;
	if (iy < borderY)			c = (float)iy / (float) borderY;
	if (iy > (int)Height - borderY)	c = (float)(borderY - (iy-(Height-borderY))) / (float) borderY;

	c*=255;
	unsigned long clr = (unsigned long) c;
	clr = (clr<<24) | (clr<<16) | (clr<<8) | clr;

	return clr;

}

void GetXY(PDIRECT3DDEVICE9 pDevice ,
		   float x , float y , float z,
		   int* p_x , int* p_y)
{
	//Create a vector
	D3DXVECTOR3 vec;
	vec.x = x;
	vec.y = y;
	vec.z = z;

	// get all the transformations
	D3DXMATRIXA16 world , view , projection ,viewport;
	pDevice->GetTransform(D3DTS_WORLD , &world);
	pDevice->GetTransform(D3DTS_VIEW , &view);
	pDevice->GetTransform(D3DTS_PROJECTION , &projection);

	viewport = D3DXMATRIXA16(
		(float)Width/2 ,	0,					0,			0,
		0,					-(float)Height/2,	0,			0,
		0,					0,					1,			0,
		(float)Width/2	,	(float)Height/2,	0,			1
	);

	// multiply the 3 matrices
	D3DXMATRIXA16 transform;
	D3DXMatrixMultiply(&transform , &world , &view);
	D3DXMatrixMultiply(&transform , &transform , &projection);
	D3DXMatrixMultiply(&transform , &transform , &viewport);

	// Tranform the vector to get the screen coords
	D3DXVECTOR4 vec_result;
	D3DXVec3Transform(&vec_result , &vec , &transform);


	*p_x = (int)vec_result.x / vec_result.w;
	*p_y = (int)vec_result.y / vec_result.w;

}

void GetScreenCenter(int* p_x , int* p_y)
{
	*p_x = Width  / 2;
	*p_y = Height / 2;
}

void RenderQuadAt(PDIRECT3DDEVICE9 pDevice , 
				  int x , int y , 
				  float size,
				  unsigned long color)
{
	// size is a percentage of the screen horizontal size
	CUSTOMVERTEX vert[6];

	int Size = (int) (size*Width);

	vert[0].x = x-Size/2;
	vert[0].y = y-Size/2;
	vert[0].z = 0;
	vert[0].rhw =1;
	vert[0].tu = 0;
	vert[0].tv = 0;
	vert[0].color = color;

	vert[1].x = x+Size/2;
	vert[1].y = y-Size/2;
	vert[1].z = 0;
	vert[1].rhw =1;
	vert[1].tu = 1;
	vert[1].tv = 0;
	vert[1].color = color;

	vert[2].x = x-Size/2;
	vert[2].y = y+Size/2;
	vert[2].z = 0;
	vert[2].rhw =1;
	vert[2].tu = 0;
	vert[2].tv = 1;
	vert[2].color = color;

	vert[3].x = x-Size/2;
	vert[3].y = y+Size/2;
	vert[3].z = 0;
	vert[3].rhw =1;
	vert[3].tu = 0;
	vert[3].tv = 1;
	vert[3].color = color;

	vert[4].x = x+Size/2;
	vert[4].y = y-Size/2;
	vert[4].z = 0;
	vert[4].rhw =1;
	vert[4].tu = 1;
	vert[4].tv = 0;
	vert[4].color = color;

	vert[5].x = x+Size/2;
	vert[5].y = y+Size/2;
	vert[5].z = 0;
	vert[5].rhw =1;
	vert[5].tu = 1;
	vert[5].tv = 1;
	vert[5].color = color;

	// set the fvf
	pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	pDevice->BeginScene();

	// render this quad
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
		2,
		(void*) vert,
		sizeof(CUSTOMVERTEX));

	pDevice->EndScene();
}

bool GetTransition()
{
	return Transition;
}

void SetTransition(bool t)
{
	Transition = t;
}


void RenderSpecificQuad(PDIRECT3DDEVICE9 pDevice , 
				  float x , float y , 
				  float width ,float height,
				  unsigned long color)
{
	// size is a percentage of the screen size
	CUSTOMVERTEX vert[6];

	float W =  (width*Width);
	float H =  (height*Height);

	vert[0].x = x*Width;
	vert[0].y = y*Height;
	vert[0].z = 0;
	vert[0].rhw =1;
	vert[0].tu = x;
	vert[0].tv = y;
	vert[0].color = color;

	vert[1].x = x*Width+W;
	vert[1].y = y*Height;
	vert[1].z = 0;
	vert[1].rhw =1;
	vert[1].tu = x+width;
	vert[1].tv = y;
	vert[1].color = color;

	vert[2].x = x*Width;
	vert[2].y = y*Height+H;
	vert[2].z = 0;
	vert[2].rhw =1;
	vert[2].tu = x;
	vert[2].tv = y+height;
	vert[2].color = color;

	vert[3].x = x*Width;
	vert[3].y = y*Height+H;
	vert[3].z = 0;
	vert[3].rhw =1;
	vert[3].tu = x;
	vert[3].tv = y+height;
	vert[3].color = color;

	vert[4].x = x*Width+W;
	vert[4].y = y*Height;
	vert[4].z = 0;
	vert[4].rhw =1;
	vert[4].tu = x+width;
	vert[4].tv = y;
	vert[4].color = color;

	vert[5].x = x*Width+W;
	vert[5].y = y*Height+H;
	vert[5].z = 0;
	vert[5].rhw =1;
	vert[5].tu = x+width;
	vert[5].tv = y+height;
	vert[5].color = color;

	// set the fvf
	pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	pDevice->BeginScene();

	// render this quad
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
		2,
		(void*) vert,
		sizeof(CUSTOMVERTEX));

	pDevice->EndScene();
}

void RenderGaussianQuad(PDIRECT3DDEVICE9 pDevice , 
				  float blur)
{
	if (!BlurEnable) 
	{
		// Multipass , Sorry!
		CUSTOMVERTEX vert[6];

		vert[0].x = 0;
		vert[0].y = 0;
		vert[0].z = 0;
		vert[0].rhw =1;
		vert[0].tu = 0;
		vert[0].tv = 0;
		vert[0].color = 0x40404040;

		vert[1].x = Width;
		vert[1].y = 0;
		vert[1].z = 0;
		vert[1].rhw =1;
		vert[1].tu = 1;
		vert[1].tv = 0;
		vert[1].color = 0x40404040;

		vert[2].x = 0;
		vert[2].y = Height;
		vert[2].z = 0;
		vert[2].rhw =1;
		vert[2].tu = 0;
		vert[2].tv = 1;
		vert[2].color = 0x40404040;

		vert[3].x = 0;
		vert[3].y = Height;
		vert[3].z = 0;
		vert[3].rhw =1;
		vert[3].tu = 0;
		vert[3].tv = 1;
		vert[3].color = 0x40404040;

		vert[4].x = Width;
		vert[4].y = 0;
		vert[4].z = 0;
		vert[4].rhw =1;
		vert[4].tu = 1;
		vert[4].tv = 0;
		vert[4].color = 0x40404040;

		vert[5].x = Width;
		vert[5].y = Height;
		vert[5].z = 0;
		vert[5].rhw =1;
		vert[5].tu = 1;
		vert[5].tv = 1;
		vert[5].color = 0x40404040;

		// First pass - the UP quad
		vert[0].tu = 0;
		vert[1].tu = 1;
		vert[2].tu = 0;
		vert[3].tu = 0;
		vert[4].tu = 1;
		vert[5].tu = 1;

		vert[0].tv = 0+blur;
		vert[1].tv = 0+blur;
		vert[2].tv = 1+blur;
		vert[3].tv = 1+blur;
		vert[4].tv = 0+blur;
		vert[5].tv = 1+blur;

		// Set blending
		pDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

		// set the fvf
		pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		pDevice->BeginScene();

		// render this quad
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
			2,
			(void*) vert,
			sizeof(CUSTOMVERTEX));

		pDevice->EndScene();

		// Second pass - the DOWN quad
		vert[0].tu = 0;
		vert[1].tu = 1;
		vert[2].tu = 0;
		vert[3].tu = 0;
		vert[4].tu = 1;
		vert[5].tu = 1;

		vert[0].tv = 0-blur;
		vert[1].tv = 0-blur;
		vert[2].tv = 1-blur;
		vert[3].tv = 1-blur;
		vert[4].tv = 0-blur;
		vert[5].tv = 1-blur;

		// Set blending
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		pDevice->SetRenderState(D3DRS_SRCBLEND  , D3DBLEND_ONE);
		pDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

		// set the fvf
		pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		pDevice->BeginScene();

		// render this quad
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
			2,
			(void*) vert,
			sizeof(CUSTOMVERTEX));

		pDevice->EndScene();

		// Third pass - the LEFT quad
		vert[0].tu = 0-blur;
		vert[1].tu = 1-blur;
		vert[2].tu = 0-blur;
		vert[3].tu = 0-blur;
		vert[4].tu = 1-blur;
		vert[5].tu = 1-blur;

		vert[0].tv = 0;
		vert[1].tv = 0;
		vert[2].tv = 1;
		vert[3].tv = 1;
		vert[4].tv = 0;
		vert[5].tv = 1;

		// Set blending
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		pDevice->SetRenderState(D3DRS_SRCBLEND  , D3DBLEND_ONE);
		pDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

		// set the fvf
		pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		pDevice->BeginScene();

		// render this quad
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
			2,
			(void*) vert,
			sizeof(CUSTOMVERTEX));

		pDevice->EndScene();

		// Fourth pass - the RIGHT quad
		vert[0].tu = 0+blur;
		vert[1].tu = 1+blur;
		vert[2].tu = 0+blur;
		vert[3].tu = 0+blur;
		vert[4].tu = 1+blur;
		vert[5].tu = 1+blur;

		vert[0].tv = 0;
		vert[1].tv = 0;
		vert[2].tv = 1;
		vert[3].tv = 1;
		vert[4].tv = 0;
		vert[5].tv = 1;

		// Set blending
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		pDevice->SetRenderState(D3DRS_SRCBLEND  , D3DBLEND_ONE);
		pDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

		// set the fvf
		pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		pDevice->BeginScene();

		// render this quad
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
			2,
			(void*) vert,
			sizeof(CUSTOMVERTEX));

		pDevice->EndScene();

		return;
	}

	// size is a percentage of the screen size
	FOURTEXTUREVERTEX vert[6];

	float W =  1.0f * Width;
	float H =  1.0f * Height;

	vert[0].x = 0;
	vert[0].y = 0;
	vert[0].z = 0;
	vert[0].rhw =1;
	vert[0].tu1 = 0;
	vert[0].tv1 = 0+blur;
	vert[0].tu2 = 0;
	vert[0].tv2 = 0-blur;
	vert[0].tu3 = 0-blur;
	vert[0].tv3 = 0;
	vert[0].tu4 = 0+blur;
	vert[0].tv4 = 0;
	vert[0].color = 0xffffffff;

	vert[1].x = W;
	vert[1].y = 0;
	vert[1].z = 0;
	vert[1].rhw =1;
	vert[1].tu1 = 1;
	vert[1].tv1 = 0+blur;
	vert[1].tu2 = 1;
	vert[1].tv2 = 0-blur;
	vert[1].tu3 = 1-blur;
	vert[1].tv3 = 0;
	vert[1].tu4 = 1+blur;
	vert[1].tv4 = 0;
	vert[1].color = 0xffffffff;

	vert[2].x = 0;
	vert[2].y = H;
	vert[2].z = 0;
	vert[2].rhw =1;
	vert[2].tu1 = 0;
	vert[2].tv1 = 1+blur;
	vert[2].tu2 = 0;
	vert[2].tv2 = 1-blur;
	vert[2].tu3 = 0-blur;
	vert[2].tv3 = 1;
	vert[2].tu4 = 0+blur;
	vert[2].tv4 = 1;
	vert[2].color = 0xffffffff;

	vert[3].x = 0;
	vert[3].y = H;
	vert[3].z = 0;
	vert[3].rhw =1;
	vert[3].tu1 = 0;
	vert[3].tv1 = 1+blur;
	vert[3].tu2 = 0;
	vert[3].tv2 = 1-blur;
	vert[3].tu3 = 0-blur;
	vert[3].tv3 = 1;
	vert[3].tu4 = 0+blur;
	vert[3].tv4 = 1;
	vert[3].color = 0xffffffff;

	vert[4].x = W;
	vert[4].y = 0;
	vert[4].z = 0;
	vert[4].rhw =1;
	vert[4].tu1 = 1;
	vert[4].tv1 = 0+blur;
	vert[4].tu2 = 1;
	vert[4].tv2 = 0-blur;
	vert[4].tu3 = 1-blur;
	vert[4].tv3 = 0;
	vert[4].tu4 = 1+blur;
	vert[4].tv4 = 0;
	vert[4].color = 0xffffffff;

	vert[5].x = W;
	vert[5].y = H;
	vert[5].z = 0;
	vert[5].rhw =1;
	vert[5].tu1 = 1;
	vert[5].tv1 = 1+blur;
	vert[5].tu2 = 1;
	vert[5].tv2 = 1-blur;
	vert[5].tu3 = 1-blur;
	vert[5].tv3 = 1;
	vert[5].tu4 = 1+blur;
	vert[5].tv4 = 1;
	vert[5].color = 0xffffffff;

	pDevice->SetRenderState(D3DRS_ZENABLE , FALSE);

	// set the fvf
	pDevice->SetFVF(D3DFVF_FOURTEXTUREVERTEX);

	/*
	// Set texture sampler states
	pDevice->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(1,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(1,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(2,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(2,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(3,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(3,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	*/
	pDevice->BeginScene();

	// set the blur shader
	pDevice->SetPixelShader(pBlurShader);

	// render this quad
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
		2,
		(void*) vert,
		sizeof(FOURTEXTUREVERTEX));

	pDevice->EndScene();

	pDevice->SetPixelShader(0);
}




