// SCenes.cpp


#include "CylinderScene.h"
#include <stdio.h>

#define SAMURAI_BEGIN		0
#define SAMURAI_STAY		4000
#define SPYROS_BEGIN		7000
#define SPYROS_STAY			10000
#define SYBEX_BEGIN			13000
#define SYBEX_STAY			16000
#define CAT_BEGIN			19000
#define CAT_STAY			22000

#define ABSOLUTE_END		38000

#define MALAKIA_SHIFT		125

// Flashbak
#define FLASHBACK_01		25625 + MALAKIA_SHIFT
#define FLASHBACK_02		25750 + MALAKIA_SHIFT
#define FLASHBACK_03		25875 + MALAKIA_SHIFT
#define FLASHBACK_04		26000 + MALAKIA_SHIFT
#define FLASHBACK_05		26125 + MALAKIA_SHIFT
#define FLASHBACK_06		26250 + MALAKIA_SHIFT
#define FLASHBACK_07		26375 + MALAKIA_SHIFT
#define FLASHBACK_08		26500 + MALAKIA_SHIFT
#define FLASHBACK_09		26625 + MALAKIA_SHIFT
#define FLASHBACK_10		26750 + MALAKIA_SHIFT
#define FLASHBACK_11		26875 + MALAKIA_SHIFT
#define FLASHBACK_12		27000 + MALAKIA_SHIFT
#define FLASHBACK_13		27125 + MALAKIA_SHIFT
#define FLASHBACK_14		27250 + MALAKIA_SHIFT
#define FLASHBACK_15		27375 + MALAKIA_SHIFT
#define FLASHBACK_16		27500 + MALAKIA_SHIFT
#define FLASHBACK_17		27625 + MALAKIA_SHIFT
#define FLASHBACK_18		27750 + MALAKIA_SHIFT
#define FLASHBACK_19		27875 + MALAKIA_SHIFT
#define THE_END				28000 + MALAKIA_SHIFT

// GLOBALS /////////////////////////////////////////////////

extern char text_buffer[];
extern DWORD frames;

extern FILE* output;
extern bool  Saving;
extern char saving_text_buffer[256];
extern CameraShot camera_buffer[1000];
extern unsigned long shots;

extern TextureManager TexMan;

float FthinousaTalantosi(float x)
{
	return (float) cos(x) * pow(2,1.0f/x);
}

/////////////////////////////////////////////////////////////

bool Scene3::Init()
{
	
	// A better pointer to the device (less redirection)
	m_pd3dDevice = m_pHostDemo->m_pd3dDevice;

	// Debug
	WriteToLog("<b>Credits SCene</b><br><hr>");

	Cylinder.CreateCylinder(1000.0f,87.0f,48,32);
	Cylinder.AllocateSpace();
	Cylinder.CalculateFaceNormals(true);
	Cylinder.FindNeighbors();
	Cylinder.CalculateNormals(true);
	Cylinder.CreateBuffers(m_pd3dDevice);
	Cylinder.RefreshBuffers();

	Cylinder2.CreateCylinder(1000.0f,87.0f,12,8);
	Cylinder2.AllocateSpace();
	Cylinder2.CalculateFaceNormals(true);
	Cylinder2.FindNeighbors();
	Cylinder2.CalculateNormals(true);
	Cylinder2.CreateBuffers(m_pd3dDevice);
	Cylinder2.RefreshBuffers();

	// Debug
	WriteToLog("<ul>");
	WriteToLog("<li>Cylinder OK</li>");
	

	Particles.CreateFrom(m_pd3dDevice , &Cylinder2);

	// Debug
	WriteToLog("<li>Particles OK</li>");

	if (!m1.LoadFrom3DS("data\\geometry\\eggplant.3ds" , "PlastiKPurple01"))
	{
		WriteToLog("<li>Could not load the material from eggplant.3ds</li>");
	}
	else
	{
		WriteToLog("<li>Material PlastiKPurple01 from eggplant.3ds - OK</li>");
	}

	// camera
	camera1.Set( -800 , 150 , -480 , D3DX_PI/2 , 0.0f , 0.0f);


	// Create the cubetexture
	if FAILED(D3DXCreateCubeTextureFromFile(          
			m_pd3dDevice,
			"data/textures/Chrome.dds",
			&CubeMap
			))
	{
		WriteToLog("<li>Could not load the chrome cubemap</li>");
	}
	else
	{
		WriteToLog("<li>Chrome.dds cubetexture - OK</li>");
	}
	m2.CreateRectangle(2,20,50,100);
	m2.CreateBuffers(m_pd3dDevice);
	m2.ApplyColor(0x00000000);
	m2.RefreshBuffers();

	Names.CreateRectangle(24,24,100,60);
	Names.CreateBuffers(m_pd3dDevice);
	Names.ApplyColor(0xffffffff);
	Names.RefreshBuffers();

	// Debug
	WriteToLog("<li>Grid is ready</li>");

	// Load all the textures here
	TexMan.LoadTexture(m_pd3dDevice , "Credits/2d gfx.png");
	TexMan.LoadTexture(m_pd3dDevice , "Credits/3d gfx.png");
	TexMan.LoadTexture(m_pd3dDevice , "Credits/Code.png");
	TexMan.LoadTexture(m_pd3dDevice , "Credits/music.png");
	TexMan.LoadTexture(m_pd3dDevice , "cow.png");
	TexMan.LoadTexture(m_pd3dDevice , "floor.png");
	TexMan.LoadTexture(m_pd3dDevice , "end.png");

	// Flashback Textures
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/001.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/002.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/003.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/004.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/005.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/006.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/007.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/008.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/009.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/010.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/011.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/012.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/013.jpg");
	TexMan.LoadTexture(m_pd3dDevice , "Flashback/014.jpg");

	// Debug
	WriteToLog("</ul>");

	return true;

}

void Scene3::Render()
{

	// *************************************************
	// CAMERA INTERFACE
	// *************************************************

	// Update the camera ONLY if we are not saving now
	if (m_pHostDemo->m_bKeys[DIK_RETURN])
	{
		Saving = true;
	}

	if (!Saving)
	{
		if (m_pHostDemo->m_bMouseL)
		{
			camera1.Update(UPDATE_YAW_PITCH , 
				m_pHostDemo->m_fMouseXAxis,
				m_pHostDemo->m_fMouseYAxis);
		}
		else if (m_pHostDemo->m_bMouseR)
		{
			camera1.Update(UPDATE_ROLL_HEIGHT , 
				m_pHostDemo->m_fMouseXAxis,
				m_pHostDemo->m_fMouseYAxis);
		}
		else
		{
			camera1.Update(UPDATE_POSITION , 
				m_pHostDemo->m_fMouseXAxis,
				m_pHostDemo->m_fMouseYAxis);
		}
	}
	else
	{
		sprintf(saving_text_buffer , "TimeDelta (ms):%d",
			m_pHostDemo->m_MouseX);
		if (m_pHostDemo->m_bMouseL) 
		{
			// Save this camera
			if (m_pHostDemo->m_MouseX > 0)
				camera1.TimeDelta = m_pHostDemo->m_MouseX;
			else
				camera1.TimeDelta = 0;

			camera_buffer[shots].x = camera1.x;
			camera_buffer[shots].y = camera1.y;
			camera_buffer[shots].z = camera1.z;
			camera_buffer[shots].yaw = camera1.yaw;
			camera_buffer[shots].pitch = camera1.pitch;
			camera_buffer[shots].roll = camera1.roll;
			camera_buffer[shots].TimeDelta = camera1.TimeDelta;

			shots++;

			// ---and---
			// go back to normal mode
			Saving = false;
		}
	}

	// delete the last keyframe if asked
	if (m_pHostDemo->m_bKeys[DIK_BACKSPACE])
	{
		if (shots>0)
			shots--;
	}
	// *************************************************
	// END CAMERA INTERFACE
	// *************************************************

	// --------------------------------------------------------------

	//********************************
	// MATRIX STUFF
	//********************************

	Cylinder.ClearTransform();
	//Cylinder.SetTransform(TRANSFORM_ROTATE , 0,D3DX_PI/2,0);

	// View Matrix
	camera1.UpdateViewMatrix(m_pd3dDevice);

	// Projection Matrix
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 4.0f/3.0f, 1.0f, 10000.0f );
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	//********************************
	// END MATRIX STUFF
	//********************************


	// Set up a white, directional light, with an oscillating direction.
	// Note that many lights may be active at a time (but each one slows down
	// the rendering of our scene). However, here we are just using one. Also,
	// we need to set the D3DRS_LIGHTING renderstate to enable lighting
	D3DXVECTOR3 vecDir;
	D3DLIGHT9 light;
	ZeroMemory( &light, sizeof(D3DLIGHT9) );
	light.Type       = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r  = 0.9f;
	light.Diffuse.g  = 0.9f;
	light.Diffuse.b  = 0.9f;
	light.Specular.r  = 0.1f;
	light.Specular.g  = 0.1f;
	light.Specular.b  = 0.1f;
	vecDir = camera1.GetDirection();
	D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &vecDir );
	light.Range       = 1000.0f;
	if (FAILED(m_pd3dDevice->SetLight( 0, &light )))
	{
		WriteToLog("Failed to set up a light");
	}///
	if (FAILED(m_pd3dDevice->LightEnable( 0, TRUE )))
	{
		WriteToLog("Failed to turn the light on");
	}

	if (FAILED(m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE )))
	{
		WriteToLog("Failed to turn on lighting");
	}


	// Finally, turn on some ambient light.
	m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );
	
	// Enable specular
	m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_LOCALVIEWER , TRUE);
	
	// Turn on Z
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
	//m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);


	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(255,0,0,0),1.0f,0);

	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MIPFILTER , D3DTEXF_LINEAR);

	m_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_NONE);
	m_pd3dDevice->SetRenderState(D3DRS_FILLMODE , D3DFILL_SOLID);
	m1.Set(m_pd3dDevice , 0);
	
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);
	TexMan.SetTexture(m_pd3dDevice , 1 , 0);
	TexMan.SetTexture(m_pd3dDevice , 0 , "cow.png");
	m2.ClearTransform();
	m2.SetTransform(TRANSFORM_SCALE , 21,18,1);
	m2.SetTransform(TRANSFORM_ROTATE , 0 , D3DX_PI/2 , 0);
	m2.SetTransform(TRANSFORM_TRANSLATE , 100,670-m_dwTime/50.0f,-470);
	m2.Render(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , TRUE);

	m_pd3dDevice->SetTexture(1, CubeMap);
		// Autocalculate cubemap coords
	m_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );
	m_pd3dDevice->SetTextureStageState(1 , D3DTSS_COLOROP , D3DTOP_ADDSMOOTH);

	m_pd3dDevice->SetSamplerState(0,D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0,D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(1,D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(1,D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	
	Cylinder.Distortion(m_dwTime*1.5);
	Cylinder.CalculateFaceNormalsFromVB();
	Cylinder.CalculateNormalsToVB();
	TexMan.SetTexture(m_pd3dDevice , 0 , "floor.png");
	Cylinder.SetTransform(TRANSFORM_ROTATE , 0 , D3DX_PI , 0);
	Cylinder.Render(m_pd3dDevice);

	TexMan.SetTexture(m_pd3dDevice , 1 , 0);

	// Set blending to invdestcolor
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);

	// Render Particles
	Cylinder2.Distortion(m_dwTime*1.5);
	Particles.UpdateFrom(&Cylinder2);
	Particles.ClearTransform();
	Particles.SetTransform(TRANSFORM_SCALE , 1.2f , 1.2f , -1.0f);
	Particles.SetTransform(TRANSFORM_ROTATE , 0 , D3DX_PI , 0);
	TexMan.SetTexture(m_pd3dDevice , 0 , "star.png");
	Particles.Render(m_pd3dDevice);

	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , TRUE);



	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	// Debug output
	// if we are saving print the number of miliseconds
	if (Saving) PutText(m_pd3dDevice,0,100,saving_text_buffer);

	sprintf(text_buffer , "FPS:  %f\n" , 
		m_pHostDemo->m_fFPS  );
	PutText(m_pd3dDevice,0,0,text_buffer);

	sprintf(text_buffer , "Shots: %d" , shots);
	PutText(m_pd3dDevice,0,80,text_buffer);

	// Set View matrix to identity
	D3DXMATRIXA16 iden;
	D3DXMatrixIdentity(&iden);
	m_pd3dDevice->SetTransform(D3DTS_WORLD , &iden);
	m_pd3dDevice->SetTransform(D3DTS_VIEW , &iden);


	unsigned long clr;

	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
	m_pd3dDevice->SetTextureStageState(0 , D3DTSS_COLOROP , D3DTOP_MODULATE);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	if (m_dwTime <SAMURAI_STAY)
	{
		clr = (unsigned long) 255.0f * (((m_dwTime-SAMURAI_BEGIN)/(float)(SAMURAI_STAY-SAMURAI_BEGIN))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		Names.SetTransform(TRANSFORM_SCALE , FthinousaTalantosi(((SAMURAI_STAY-SAMURAI_BEGIN)-(m_dwTime-SAMURAI_BEGIN))/(float)(SAMURAI_STAY-SAMURAI_BEGIN)),1,1);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/code.png");
		Names.VortexRectangle(((SAMURAI_STAY-SAMURAI_BEGIN)-(m_dwTime-SAMURAI_BEGIN))/(float)(SAMURAI_STAY-SAMURAI_BEGIN));
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < SPYROS_BEGIN)
	{
		clr = (unsigned long) 255.0f * (1.0f - ((m_dwTime-SAMURAI_STAY)/(float)(SPYROS_BEGIN-SAMURAI_STAY))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/code.png");
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < SPYROS_STAY)
	{
		clr = (unsigned long) 255.0f * (((m_dwTime-SPYROS_BEGIN)/(float)(SPYROS_STAY-SPYROS_BEGIN))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		Names.SetTransform(TRANSFORM_SCALE , FthinousaTalantosi(((SPYROS_STAY-SPYROS_BEGIN)-(m_dwTime-SPYROS_BEGIN))/(float)(SPYROS_STAY-SPYROS_BEGIN)),1,1);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/3d gfx.png");
		Names.VortexRectangle(((SPYROS_STAY-SPYROS_BEGIN)-(m_dwTime-SPYROS_BEGIN))/(float)(SPYROS_STAY-SPYROS_BEGIN));
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < SYBEX_BEGIN)
	{
		clr = (unsigned long) 255.0f * (1.0f - ((m_dwTime-SPYROS_STAY)/(float)(SYBEX_BEGIN-SPYROS_STAY))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/3d gfx.png");
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < SYBEX_STAY)
	{
		clr = (unsigned long) 255.0f * (((m_dwTime-SYBEX_BEGIN)/(float)(SYBEX_STAY-SYBEX_BEGIN))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		Names.SetTransform(TRANSFORM_SCALE , FthinousaTalantosi(((SYBEX_STAY-SYBEX_BEGIN)-(m_dwTime-SYBEX_BEGIN))/(float)(SYBEX_STAY-SYBEX_BEGIN)),1,1);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/2d gfx.png");
		Names.VortexRectangle(((SYBEX_STAY-SYBEX_BEGIN)-(m_dwTime-SYBEX_BEGIN))/(float)(SYBEX_STAY-SYBEX_BEGIN));
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < CAT_BEGIN)
	{
		clr = (unsigned long) 255.0f * (1.0f - ((m_dwTime-SYBEX_STAY)/(float)(CAT_BEGIN-SYBEX_STAY))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/2d gfx.png");
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < CAT_STAY)
	{
		clr = (unsigned long) 255.0f * (((m_dwTime-CAT_BEGIN)/(float)(CAT_STAY-CAT_BEGIN))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		Names.SetTransform(TRANSFORM_SCALE , FthinousaTalantosi(((CAT_STAY-CAT_BEGIN)-(m_dwTime-CAT_BEGIN))/(float)(CAT_STAY-CAT_BEGIN)),1,1);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/music.png");
		Names.VortexRectangle(((CAT_STAY-CAT_BEGIN)-(m_dwTime-CAT_BEGIN))/(float)(CAT_STAY-CAT_BEGIN));
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < THE_END)
	{
		clr = (unsigned long) 255.0f * (1.0f - ((m_dwTime-CAT_STAY)/(float)(THE_END-CAT_STAY))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		Names.ApplyColor(clr);
		Names.RefreshBuffers();
		Names.ClearTransform();
		Names.SetTransform(TRANSFORM_TRANSLATE , 0,10,100);
		TexMan.SetTexture(m_pd3dDevice , 0 , "Credits/music.png");
		Names.Render(m_pd3dDevice);
	}
	else
	if (m_dwTime < ABSOLUTE_END)
	{
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
		clr = (unsigned long) 255.0f * (1.0f - ((m_dwTime-THE_END)/(float)(ABSOLUTE_END-THE_END))); 
		clr = (clr << 24) | (clr<<16) | (clr<<8) | clr;
		TexMan.SetTexture(m_pd3dDevice , 0 , "end.png");
		RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,clr);
	}

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	// FlashBack Shit
	if (m_dwTime > FLASHBACK_01)
	{
		if (m_dwTime < FLASHBACK_02)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/001.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_03)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/002.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_04)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/003.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_05)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/004.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_06)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/005.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_07)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/006.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_08)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/007.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_09)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/008.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_10)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/009.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_11)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/010.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_12)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/011.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_13)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/012.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_14)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/013.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_15)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/014.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_16)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/004.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_17)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/006.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_18)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/008.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < FLASHBACK_19)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/010.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
		else
		if (m_dwTime < THE_END)
		{
			TexMan.SetTexture(m_pd3dDevice , 0 , "Flashback/012.jpg");
			RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		}
	}

	m_pd3dDevice->EndScene();

	// Present the sin!
	m_pd3dDevice->Present(NULL,NULL,NULL,NULL);
	
}

void Scene3::Release()
{



	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);

	m_pd3dDevice = NULL;
}
