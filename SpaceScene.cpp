// SCenes.cpp


#include "SpaceScene.h"
#include <stdio.h>

#define LEX_BEGIN			0
#define LEX_STAY			7000
#define LEX_FADE			8000
#define REVERSED_FADE		12000
#define PATRAS_BEGIN		13000
#define FIRE_BEGIN			15000
#define FIRE_STAY			20000

#define FIRE_FADE			35000

#define SAGAN_BEGIN			33000
#define SAGAN_STAY			37000
#define SAGAN_FADE			40000
#define TRANSITION_BEGIN	69000
#define ABSOLUTE_END		70000



//#define CAMERA_RECORDING

// GLOBALS /////////////////////////////////////////////////

extern char text_buffer[];
extern DWORD frames;

unsigned char palette_copy[256];

extern TextureManager TexMan;

/////////////////////////////////////////////////////////////

bool SpaceScene::Init()
{
	
	// A better pointer to the device (less redirection)
	m_pd3dDevice = m_pHostDemo->m_pd3dDevice;

	// Debug
	WriteToLog("Initializing <b>Space SCene</b><br><hr><ul>");

	if (!Pioneer.LoadFromS3D("data\\geometry\\pioneer.s3d",m_pd3dDevice))
	{
		WriteToLog("<li>Couldn't load pioneer.s3d</li>");
		return 0;
	}
	if(!Sympan.LoadFromS3D("data\\geometry\\sympan.s3d",m_pd3dDevice))
	{
		WriteToLog("<li>Couldn't load sympan.s3d</li>");
		return 0;
	}
	if(!Man.LoadFromS3D("data/geometry/man.s3d" , m_pd3dDevice))
	{
		WriteToLog("<li>Couldn't load man.s3d</li>");
		return 0;
	}
	if(!Woman.LoadFromS3D("data/geometry/woman.s3d" , m_pd3dDevice))
	{
		WriteToLog("<li>Couldn't load woman.s3d</li>");
		return 0;
	}

	// Create the cubetexture
	if (FAILED(D3DXCreateCubeTextureFromFile(          
			m_pd3dDevice,
			"data\\textures\\sympan.dds",
			&CubeMap
			))) 
	{
		WriteToLog("<li>Failed to load the cubemap sympan.dds</li>");
	}
	else
	{
		WriteToLog("<li>Cubemap sympan.dds - OK</li>");
	}

	// Create the quad
	m1.CreateRectangle(32,32,14,14*3/4);
	m1.CreateBuffers(m_pd3dDevice);
	m1.ApplyColor(0xffffffff);
	m1.RefreshBuffers();

	WriteToLog("<li>Grid is ready</li>");


	/////////// FIRE STUFF ////////////////
	// Create an empty texture
	if(FAILED(m_pd3dDevice->CreateTexture(256,256,
						1,
						0,//D3DUSAGE_WRITEONLY,
						D3DFMT_A8R8G8B8,
						D3DPOOL_MANAGED ,
						&p_empty_tex,
						0)))
	{
		WriteToLog("<li>Failed to create an empty texture for the fire</li>");
	}
	else
	{
		WriteToLog("<li>Fire texture is ready</li>");
	}


	// Init the palette
	for (int x=0;x<256;x++)
	{
		int alpha = (int)2.5f*x;
		if (alpha>255) alpha = 255;
		if (x<120) palette[x] = D3DCOLOR_ARGB(alpha,0,x,128-(x/2));
		else palette[x] = D3DCOLOR_ARGB(alpha,255-x,x,60);
	}
	// copy the palette 
	memcpy(palette_copy , palette , 256);

	// clear the texture 
	// Lock a texture to render to it
	D3DLOCKED_RECT LockedRect;
	if (FAILED(p_empty_tex->LockRect(0,
					&LockedRect,
					NULL,
					0)))
	{
		WriteToLog("<li>Failed to lock the fire texture</li>");
		return 0;
	}
	else
	{
		WriteToLog("<li>Fire texture locked ok</li>");
	}

	DWORD* p_pixels = (DWORD*) LockedRect.pBits;
	// CAUTION!! The Pitch is in BYTES!!!
	int pitch = LockedRect.Pitch / 4;

	for (int iy=0;iy<256;iy++)
	{
		for (int ix=0;ix<256;ix++)
		{
			p_pixels[ix+iy*pitch] = palette[0];
		}
	}

	p_empty_tex->UnlockRect(0);

	// Init the buffers;
	fire = fire_buffer;
	scratch = scratch_buffer;

	// Turn on bilinear filtering
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);

	// Create a second texture to be used as a render target
	if (FAILED(m_pd3dDevice->CreateTexture(512,512,1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&tex_transition,
		NULL)))
	{
		WriteToLog("<li>Render texture could not be created!</li>");
	}
	else
	{
		WriteToLog("<li>Render target texture created OK</li>");
	}

	TexMan.LoadTexture(m_pd3dDevice , "Sagan.png");
	TexMan.LoadTexture(m_pd3dDevice , "Noise.jpg");


	// ** Camera Interface **
#ifdef CAMERA_RECORDING
	// camera
	camera1.Set(0,30,-400,0,0.0f,0.0f);
	// open the output file
	output = fopen("data/geometry/space.cam" , "w+b");
#else
	// ** Camera Replay **
	if(!Seq1.Load("data/geometry/space.cam"))
	{
		WriteToLog("<li>Camera Sequence -space.cam- couldn't be loaded</li>");
	}
	else
	{
		WriteToLog("<li>Camera Sequence -space.cam- ok</li>");
	}
#endif

	// Debug
	WriteToLog("</ul>");

	return true;

}

void SpaceScene::Render()
{

	D3DXMATRIXA16 matWorld,matWorld2,matWorld3;

	// ** Camera Interface **
#ifdef CAMERA_RECORDING

	static unsigned long RecordingTime = 0;
	static unsigned long LastRecordingTime = 0;

	// Update the camera ONLY if we are not saving now
	if (m_pHostDemo->m_bKeys[DIK_RETURN])
	{
		Saving = true;
	}

	if (!Saving)
	{
		// update m_dwTime
		m_dwTime = RecordingTime;

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
		sprintf(saving_text , "TimeDelta (ms):%d",
			m_pHostDemo->m_MouseX);

		// update m_dwTime
		m_dwTime = RecordingTime + m_pHostDemo->m_MouseX;

		if (m_pHostDemo->m_bMouseL) 
		{
			//Update RecordingTime
			LastRecordingTime = RecordingTime;
			RecordingTime += m_pHostDemo->m_MouseX;

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
		{
			RecordingTime = LastRecordingTime;
			shots--;
		}
		else RecordingTime = 0;
	}

	// View Matrix
	camera1.UpdateViewMatrix(m_pd3dDevice);

	// Always not finished when recording
	m_bFinished = false;

#else
	// ** Camera Replay **
	Seq1.Cerp(m_dwTime);
	Seq1.UpdateViewMatrix(m_pd3dDevice);

#endif
	// ** End Camera Interface **

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
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	// Turn off blending
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);

	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MIPFILTER , D3DTEXF_LINEAR);
	
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_NONE);

	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	m_pd3dDevice->SetTexture(1,NULL);

	if (m_dwTime > FIRE_BEGIN)
	{
		// Render sympan
		Sympan.ClearTransform();
		Sympan.SetTransform(TRANSFORM_SCALE , 600 , 600 , 600);
		Sympan.Render(m_pd3dDevice);

		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , TRUE);


		
		// Autocalculate cubemap coords
		m_pd3dDevice->SetTexture(1 , CubeMap);
		m_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 
			 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );
		m_pd3dDevice->SetTextureStageState(1 , D3DTSS_COLOROP , D3DTOP_ADDSMOOTH);
		m_pd3dDevice->SetSamplerState(1 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
		m_pd3dDevice->SetSamplerState(1 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);

		// render pioneer
		Pioneer.ClearTransform();
		//Pioneer.SetTransform(TRANSFORM_SCALE , 10 , 10 , 10);
		Pioneer.SetTransform(TRANSFORM_ROTATE , -D3DX_PI/3.9f ,0 , 0);
		Pioneer.SetTransform(TRANSFORM_TRANSLATE , 0 , 27 , -400.0f);
		Pioneer.Render(m_pd3dDevice);

		// Render man
		Man.ClearTransform();
		Man.SetTransform(TRANSFORM_SCALE , -3.6f , 3.6f , 3.6f);
		Man.SetTransform(TRANSFORM_ROTATE , -D3DX_PI/2 , -D3DX_PI , 0);
		Man.SetTransform(TRANSFORM_TRANSLATE , 3.33f,30.8f,-390);
		Man.Render(m_pd3dDevice);

		// Render Woman
		Woman.ClearTransform();
		Woman.SetTransform(TRANSFORM_SCALE , 3.6f , 3.6f , 3.6f);
		Woman.SetTransform(TRANSFORM_ROTATE , 0 , D3DX_PI/2.5f , 0);
		Woman.SetTransform(TRANSFORM_TRANSLATE , 4.1f,30.8f,-391.0f);
		Woman.Render(m_pd3dDevice);
	}


	if (m_dwTime > FIRE_BEGIN && m_dwTime < FIRE_FADE) UpdateFire();
	if (m_dwTime > LEX_STAY && m_dwTime < FIRE_FADE) RenderFire();

	if (m_dwTime<REVERSED_FADE) RenderReversed();
	else 
	if (m_dwTime < FIRE_BEGIN) RenderPatras();

	if (m_dwTime < LEX_FADE)
	{
		unsigned long lex_clr;
		float lex_fade;
		if (m_dwTime < LEX_STAY) lex_fade = 0;
		else lex_fade = (m_dwTime - LEX_STAY) / (float) (LEX_FADE - LEX_STAY);
	
		lex_clr = 255* (1-lex_fade);
		lex_clr = (lex_clr<<24)|0x00ffffff;//(lex_clr<<24) | (lex_clr<<16) | (lex_clr<<8) | lex_clr;
		TexMan.SetTexture(m_pd3dDevice , 0 , "lex.png");
		m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCALPHA);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA);

		// Texture stage states
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );  
 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
 
		RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,lex_clr);
		m_pd3dDevice->SetTextureStageState(0 , D3DTSS_COLOROP , D3DTOP_MODULATE);
	}

	m1.ApplyColor(0xffffffff);
	m1.RefreshBuffers();

	// Debug output
	// if we are saving print the number of miliseconds
	if (Saving) PutText(m_pd3dDevice,0,100,saving_text);

	sprintf(text_buffer , "FPS:  %f" , 
		m_pHostDemo->m_fFPS );
	PutText(m_pd3dDevice,0,0,text_buffer);

	sprintf(text_buffer , "Shots: %d\nTime: %d/%d" , shots , m_dwTime , m_dwDuration);
	PutText(m_pd3dDevice,0,180,text_buffer);

	D3DXMATRIXA16 v;
	
	// render the Sagan
	if (m_dwTime > SAGAN_BEGIN && m_dwTime < SAGAN_FADE)
	{
		// set view as identity
		D3DXMatrixIdentity(&v);
		m_pd3dDevice->SetTransform(D3DTS_VIEW , &v);

		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE ,TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);
		TexMan.SetTexture(m_pd3dDevice, 0 , "Sagan.png");

		// Close zwrites and zbuffer
		m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);

		m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);

		// find the sagan color
		unsigned long sagan_color;
		float sagan_float;
		if (m_dwTime < SAGAN_STAY)
		{
			sagan_float = (float) (m_dwTime - SAGAN_BEGIN) / (float) (SAGAN_STAY - SAGAN_BEGIN);
		}
		else
		{
			sagan_float = 1.0f - (float) (m_dwTime - SAGAN_STAY) / (float) (SAGAN_FADE - SAGAN_STAY);
		}
		sagan_float *= 255;
		sagan_color = (unsigned long) sagan_float;
		sagan_color = (sagan_color<<24) | (sagan_color<<16) | (sagan_color<<8) | sagan_color;

		m1.ApplyColor(sagan_color);
		m1.Distort(m_dwTime / 1000.0f);

		m1.ClearTransform();
		m1.SetTransform(TRANSFORM_TRANSLATE , 0 , 0 , 11);
		m1.SetTransform(TRANSFORM_SCALE , 1 , 1 , 1);

		m1.Render(m_pd3dDevice);

	}




	if (m_dwTime > TRANSITION_BEGIN)
	{
		// render the transition
		RenderToTexture(tex_transition , m_pd3dDevice);

			SetTransition(true);
			m_pNextScene->m_dwTime = m_dwTime - TRANSITION_BEGIN;
			m_pNextScene->Render();

		EndRenderToTexture(m_pd3dDevice);
		SetTransition(false);

		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE ,TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCALPHA);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA);
		m_pd3dDevice->SetTexture(0 , tex_transition);

		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
 

		m_pd3dDevice->BeginScene();
		
		// set view as identity
		D3DXMatrixIdentity(&v);
		m_pd3dDevice->SetTransform(D3DTS_VIEW , &v);

		// Close zwrites and zbuffer
		m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);

		// Render
		m1.ClearTransform();
		m1.SetTransform(TRANSFORM_TRANSLATE , 0 , 0 , 11);
		float transition_time = (float)(m_dwTime - TRANSITION_BEGIN) / (float) (ABSOLUTE_END - TRANSITION_BEGIN);
		unsigned long trans_color = (unsigned long) (255 * transition_time);
		trans_color = (trans_color<<24) | (trans_color<<16) | (trans_color<<8) | trans_color;
		m1.ApplyColor(trans_color);
		m1.VortexRectangle(1.5f*(1.0f-transition_time));

		m1.Render(m_pd3dDevice);

		m_pd3dDevice->EndScene();
	}

	// Present the sin!
	m_pd3dDevice->Present(NULL,NULL,NULL,NULL);
	

}

void SpaceScene::Release()
{


#ifdef CAMERA_RECORDING
	// Save the camera keyframes
	fwrite(&shots , sizeof(unsigned long) , 1 , output);
	fwrite(camera_buffer , sizeof(CameraShot) , shots , output);
	fclose(output);
#endif

	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);


	Pioneer.Release();
	Sympan.Release();
	m1.Release();

	m_pd3dDevice = NULL;
}

void SpaceScene::UpdateFire()
{
	// ########## FIRE SHIT ########### //
	static DWORD time = 0;
	static DWORD last_time = 0;

	time = m_dwTime;

	DWORD time_delta = time - last_time;

	// Render only 50 fps
	if (time_delta>=22)
	{

		// Fill it with random fuel
		for (int fuel =0;fuel<256;fuel++)
		{
			// A number between (-32 , 32)
			int f = rand()%80 - 40;

			int value1;

			value1 = scratch[(255<<8)+fuel];

			value1 += f; 

			if (value1<0)value1 =0;
			if (value1>255) value1 = 255;

			scratch[(255<<8)+fuel] = value1;
		}
		// Add some random fuel to the end as well
		for (fuel=0;fuel<200;fuel++)
		{
			int y = 255 - rand()%10;
			int x = rand()%255;

			int f = rand()%80 - 40;

			int value1;

			value1 = scratch[(y<<8)+x];

			value1 += f; 

			if (value1<0)value1 =0;
			if (value1>255) value1 = 255;

			scratch[(y<<8)+x] = value1;

		}
		

		// Render the fire
		// First write to the scratch buffer
		for (int fy=0;fy<256;fy++)
		{
			for (int fx=0;fx<256;fx++)
			{
				unsigned char fire_left , fire_right ,
							  fire_top , fire_bottom ;
				int result_color;
				int		x_plus1,x_minus1,
						y_plus1,y_minus1;

				x_plus1 = fx+rand()%3; if(x_plus1 > 255) x_plus1 = 0;
				x_minus1 = fx-rand()%3; if(x_minus1 < 0) x_minus1 = 255;
				y_plus1 = fy+1; if(y_plus1 > 255) y_plus1 = 255;
				y_minus1 = fy-1; if(y_minus1 < 0) y_minus1 = 0;

				fire_left = fire[x_minus1 + (fy << 8)];
				fire_right = fire[x_plus1 + (fy << 8)];
				fire_top = fire[fx + (y_minus1 << 8)];
				fire_bottom = fire[fx + (y_plus1 << 8)];

				// Calculate the result color
				result_color = fire_left + fire_right + fire_top 
					+ fire_bottom;
				result_color >>= 2;
				// Cool the fire
				if ( ! (rand()%400) )result_color--;
				if (result_color < 0) result_color = 0;

				// Write the result color to the scratch buffer
				// One row above
				scratch[fx+(y_minus1 << 8)] = result_color;
			}
		}

		// Now there is only the last row to fill


		// Now swap the fire buffers!
		swap = fire;
		fire = scratch;
		scratch = swap;

		// Lock a texture to render to it
		D3DLOCKED_RECT LockedRect;
		p_empty_tex->LockRect(0,
						&LockedRect,
						NULL,
						0);

		DWORD* p_pixels = (DWORD*) LockedRect.pBits;
		// CAUTION!! The Pitch is in BYTES!!!
		int pitch = LockedRect.Pitch / 4;


		// Write to the texture
		for (int y=0;y<256;y++)
		{
			for (int x=0;x<256;x++)
			{
				p_pixels[x+y*pitch] = palette[fire[x+256*y]];
			}
		}


		// Unlock the texture
		p_empty_tex->UnlockRect(0);

	}	// End if (fps > 50)

}

void SpaceScene::RenderFire()
{
	m1.ClearTransform();

	// Turn on blending
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCALPHA);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA);

	// Set the texture
	m_pd3dDevice->SetTexture(0,p_empty_tex);
	m_pd3dDevice->SetTexture(1 , 0);

	// turn off lighting
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);

	// set view as identity
	D3DXMATRIXA16 v;
	D3DXMatrixIdentity(&v);
	m_pd3dDevice->SetTransform(D3DTS_VIEW , &v);

	// Close zwrites and zbuffer
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);

	// Render
	m1.SetTransform(TRANSFORM_TRANSLATE , 0 , 0 , 11);
	m1.Render(m_pd3dDevice);

	// Turn on blending
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_INVSRCALPHA);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_SRCALPHA);

	m1.Render(m_pd3dDevice);

	if (m_dwTime < SAGAN_BEGIN)
	{

		// Render Noise
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

		TexMan.SetTexture(m_pd3dDevice , 0 , "Noise.jpg");
		RenderSpecificQuad(m_pd3dDevice , 
			0 , (1.0f * sin(m_dwTime/1000.0f) +1.0f)/2 , 1 , 0.1f , 0x88888888);

		RenderSpecificQuad(m_pd3dDevice , 
			0 , (1.0f * sin(m_dwTime/1100.0f) +1.0f)/2 , 1 , 0.05f , 0x88888888);

		RenderSpecificQuad(m_pd3dDevice , 
			0 , (1.0f * sin(m_dwTime/700.0f) +1.0f)/2 , 1 , 0.17f , 0x88888888);
	}

}

void SpaceScene::RenderReversed()
{

	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	// set view as identity
	D3DXMATRIXA16 v;
	D3DXMatrixIdentity(&v);
	m_pd3dDevice->SetTransform(D3DTS_VIEW , &v);

	float fade_two;

	if (m_dwTime < LEX_FADE) fade_two = 0;
	else
	if (m_dwTime < REVERSED_FADE) fade_two = (m_dwTime-LEX_FADE)/(float)(REVERSED_FADE - LEX_FADE);
	else fade_two = 1;

	unsigned long clr;
	clr = 255 * (1-fade_two);
	clr = (clr<<24) | (clr<<16) | (clr<<8) | clr;

	// React 3 stuff
	// Render "R"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -4,2,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/R.png");
	m1.Render(m_pd3dDevice);

	// Render "E"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -4 + fade_two,0.5+1.5f * fade_two,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/E.png");
	m1.Render(m_pd3dDevice);

	// render "act 3"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 4*0.08f , 0.08f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -0.31f*fade_two,-1+3.05f*fade_two,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/act3.png");
	m1.Render(m_pd3dDevice);

	// render "eversed"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 7*0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , 0,2.0f,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/eversed.png");
	m1.ApplyColor(clr);
	m1.RefreshBuffers();
	m1.Render(m_pd3dDevice);

	// render "ngineers"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 7*0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , 0,0.5f,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/ngineers.png");
	m1.Render(m_pd3dDevice);

	clr = 255 * fade_two;
	clr = (clr<<24) | (clr<<16) | (clr<<8) | clr;

	// render "Patras 2004"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 4*0.08f , 0.08f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , 0,1,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/patras.png");
	m1.ApplyColor(clr);
	m1.RefreshBuffers();
	m1.Render(m_pd3dDevice);
}

void SpaceScene::RenderPatras()
{

	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	// set view as identity
	D3DXMATRIXA16 v;
	D3DXMatrixIdentity(&v);
	m_pd3dDevice->SetTransform(D3DTS_VIEW , &v);

	float fade_two;

	if (m_dwTime < PATRAS_BEGIN) fade_two = 0;
	else
	if (m_dwTime < FIRE_BEGIN) fade_two = (m_dwTime-PATRAS_BEGIN)/(float)(FIRE_BEGIN - PATRAS_BEGIN);
	else fade_two = 1;

	unsigned long clr;
	clr = 255 * (1-fade_two);
	clr = (clr<<24) | (clr<<16) | (clr<<8) | clr;

	m1.ApplyColor(clr);
	m1.RefreshBuffers();

	// React 3 stuff
	// Render "R"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -4,2,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/R.png");
	m1.Render(m_pd3dDevice);

	// Render "E"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 0.07f , 0.07f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -4 + 1,0.5+1.5f * 1,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/E.png");
	m1.Render(m_pd3dDevice);

	// render "act 3"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 4*0.08f , 0.08f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , -0.31f*1,-1+3.05f*1,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/act3.png");
	m1.Render(m_pd3dDevice);

	// render "Patras 2004"
	m1.ClearTransform();
	m1.SetTransform(TRANSFORM_SCALE , 4*0.08f , 0.08f , 1);
	m1.SetTransform(TRANSFORM_TRANSLATE , 0,1,10);
	TexMan.SetTexture(m_pd3dDevice , 0 ,"act3/patras.png");
	m1.Render(m_pd3dDevice);

}




