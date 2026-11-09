// SCenes.cpp


#include "ParticleScene.h"
#include <stdio.h>

float RandomNumber(float from , float to)
{
	float r = rand()%1000 / 1000.0f;
	float dif = to - from;

	return from + dif * r;

}

#define TRANSITION_DIFF		1000

#define NEGATIVE1_BEGIN		30000
#define NEGATIVE1_END		40000
#define NEGATIVE2_BEGIN	44700
#define NEGATIVE2_END		49900

//#define CAMERA_RECORDING

// GLOBALS /////////////////////////////////////////////////

extern char text_buffer[];
extern DWORD frames;

extern TextureManager TexMan;


/////////////////////////////////////////////////////////////

bool Scene4::Init()
{
	
	// A better pointer to the device (less redirection)
	m_pd3dDevice = m_pHostDemo->m_pd3dDevice;

	// Debug
	WriteToLog("<b>Human Shadow Scene</b><hr><ul>");

	// Init the billboard mesh
	if (!Billboard.Init(m_pd3dDevice , 250))
	{
		WriteToLog("<li>Failed to init the billboard mesh</li>");
		return 0;
	}
	else
	{
		WriteToLog("<li>Billboard mesh - OK</li>");
	}

	// set 1000 particles
	for (int i=0;i<250;i++)
	{
		unsigned char r = RandomNumber(30,200);
		unsigned char g = RandomNumber(0,0);
		unsigned char b = RandomNumber(30,150);
		unsigned long color = (r<<16) | (g<<8) | b;

		float radius = RandomNumber(20,50);
		float zrot   = RandomNumber(0,2*D3DX_PI);
		float xrot   = RandomNumber(0,2*D3DX_PI);

		D3DXVECTOR3  pos;
		pos.x = radius;
		pos.y=0;
		pos.z=0;
		D3DXMATRIXA16 rotation ;
		D3DXMatrixRotationYawPitchRoll(&rotation , 0 , xrot , zrot);
		D3DXVec3TransformCoord(&pos , &pos , &rotation);


		Billboard.ParticleList[i].x=pos.x;
		Billboard.ParticleList[i].y=pos.y;
		Billboard.ParticleList[i].z=pos.z;
		Billboard.ParticleList[i].size=RandomNumber(20,60);
		Billboard.ParticleList[i].color=color;
		Billboard.ParticleList[i].alive=true;
	}

	if (!shadow.LoadFromS3D("data/geometry/shadow.s3d" , m_pd3dDevice))
	{
		WriteToLog("<li>Failed to load shadow.s3d</li>");
	}
	else
	{
		WriteToLog("<li>shadow.s3d - loaded OK</li>");
	}
	if (!room.LoadFromS3D("data/geometry/room.s3d" , m_pd3dDevice))
	{
		WriteToLog("<li>Failed to load room.s3d</li>");
	}
	else
	{
		WriteToLog("<li>room.s3d - loaded OK</li>");
	}
	pRoom = room.GetObject("Room_12_Sh");
	p_matRoom = room.MaMa.GetMaterial( pRoom->MaterialName);
	pRoom->InvertNormals();
	pRoom->RefreshBuffers();

	if (!Star.Init(m_pd3dDevice , 1))
	{
		WriteToLog("<li>Star billboard failed to init</li>");
	}
	else
	{
		WriteToLog("<li>Star billboard - OK</li>");
	}
	Star.ParticleList[0].alive = true;
	Star.ParticleList[0].size = 1000;
	Star.ParticleList[0].x = 0;
	Star.ParticleList[0].y = 0;
	Star.ParticleList[0].z = 0;
	Star.ParticleList[0].color = 0xffffffff;

	// ** Camera Interface **
#ifdef CAMERA_RECORDING
	// camera
	camera1.Set( 0 , 0 , -1000 , 0.0f , 0.0f , 0.0f);
	// open the output file
	output = fopen("data/geometry/shadow.cam" , "w+b");
#else
	// ** Camera Replay **
	if (!Seq1.Load("data/geometry/shadow.cam"))
	{
		WriteToLog("<li>Failed to load shadow.cam</li>");
	}
	else
	{
		WriteToLog("<li>Camera Sequence shadow.cam - OK</li>");
	}
#endif

	// create a render target for the blurring effect
	if (FAILED(m_pd3dDevice->CreateTexture(512,512,1,
			D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&render_target,
			NULL)))
		{
			WriteToLog("<li>Render texture could not be created!</li>");
		}
		else
		{
			WriteToLog("<li>Render target texture - OK</li>");
		}

	// Debug
	WriteToLog("</ul>");

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	return true;

}

void Scene4::Render()
{
	if (!GetTransition()) RenderToTexture(render_target , m_pd3dDevice);
	RenderEverything();
	if (!GetTransition()) EndRenderToTexture(m_pd3dDevice);

	if (!GetTransition())
	{
		float blur_level = m_pHostDemo->m_MouseX / 10000.0f;

		m_pd3dDevice->SetTexture(0 , render_target);
		m_pd3dDevice->SetTexture(1 , render_target);
		m_pd3dDevice->SetTexture(2 , render_target);
		m_pd3dDevice->SetTexture(3 , render_target);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
		//RenderSpecificQuad(m_pd3dDevice , 0,0,1,1,0xffffffff);
		RenderGaussianQuad(m_pd3dDevice , blur_level);
	
		m_pd3dDevice->SetTexture(0,0);
		m_pd3dDevice->SetTexture(1,0);
		m_pd3dDevice->SetTexture(2,0);
		m_pd3dDevice->SetTexture(3,0);
	}

	// Present the sin!
	if (!GetTransition()) m_pd3dDevice->Present(NULL,NULL,NULL,NULL);
	
}

void Scene4::RenderEverything()
{
	static bool FirstTime = true;
	if (FirstTime && (!GetTransition()))
	{
		FirstTime = false;
		m_dwTime = 1000;
	}

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
		sprintf(saving_text_buffer , "TimeDelta (ms):%d",
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
	if (GetTransition) Seq1.Cerp(m_dwTime);
	else Seq1.Cerp(m_dwTime+TRANSITION_DIFF);
	Seq1.UpdateViewMatrix(m_pd3dDevice);

#endif
	// ** End Camera Interface **

	// --------------------------------------------------------------

	//********************************
	// MATRIX STUFF
	//********************************

	D3DXMATRIXA16 world,view;
	// just leave it as identity
	D3DXMatrixIdentity(&world);
	m_pd3dDevice->SetTransform(D3DTS_WORLD , &world);

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
	light.Range       = 10000.0f;
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
	

	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(255,95,95,95),1.0f,0);

	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MIPFILTER , D3DTEXF_LINEAR);

	m_pd3dDevice->SetRenderState(D3DRS_FILLMODE , D3DFILL_SOLID);

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE ,TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING ,TRUE);

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_CCW);

	shadow.ClearTransform();
	shadow.SetTransform(TRANSFORM_TRANSLATE , 0,-1,0);
	shadow.SetTransform(TRANSFORM_ROTATE , -D3DX_PI/2 , D3DX_PI , 0);
	shadow.SetTransform(TRANSFORM_SCALE ,300 , 300 , 300);
	shadow.Render(m_pd3dDevice);

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_CW);

	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING ,FALSE);

	m_pd3dDevice->SetTexture(1,0);

	/*
	room.ClearTransform();
	room.SetTransform(TRANSFORM_SCALE , 1000 , 1000 , 1000);
	room.Render(m_pd3dDevice);
	*/
	p_matRoom->Set(m_pd3dDevice , 0);
	if (GetTransition())
	{
		pRoom->Twist(0.1f,m_dwTime/60.0f);
	}
	else
	{
		pRoom->Twist(0.1f,(m_dwTime+TRANSITION_DIFF)/30.0f);
	}
	pRoom->ClearTransform();
	pRoom->SetTransform(TRANSFORM_SCALE , 1000 , 1000 , 1000);
	pRoom->Render(m_pd3dDevice);

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_NONE);
	m_pd3dDevice->SetRenderState(D3DRS_FILLMODE , D3DFILL_SOLID);
	
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	// Turn on Z
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);


	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , FALSE);
	D3DXMATRIXA16 Iden;
	D3DXMatrixIdentity(&Iden);
	m_pd3dDevice->SetTransform(D3DTS_WORLD , &Iden);
	TexMan.SetTexture(m_pd3dDevice , 0 , "star.png");
	Billboard.ClearTransform();
	Billboard.SetTransform(TRANSFORM_SCALE , 9 , 9 , 9);
	if (GetTransition())
	{
		Billboard.SetTransform(TRANSFORM_ROTATE , m_dwTime/600.0f,m_dwTime/700.0f,0);
	}
	else
	{
		Billboard.SetTransform(TRANSFORM_ROTATE , (m_dwTime+TRANSITION_DIFF)/600.0f,m_dwTime/700.0f,0);
	}
	Billboard.Render(m_pd3dDevice);



	Billboard.ClearTransform();
	Billboard.SetTransform(TRANSFORM_SCALE , 9 , 9 , 9);
	if (GetTransition())
	{
		Billboard.SetTransform(TRANSFORM_ROTATE , m_dwTime/700.0f,m_dwTime/630.0f,0);
	}
	else
	{
		Billboard.SetTransform(TRANSFORM_ROTATE , (m_dwTime+TRANSITION_DIFF)/700.0f,m_dwTime/630.0f,0);
	}
	Billboard.Render(m_pd3dDevice);

	Star.ClearTransform();
	TexMan.SetTexture(m_pd3dDevice , 0 , "star.png");
	Star.Render(m_pd3dDevice);

	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE , TRUE);

	// Debug output
	// if we are saving print the number of miliseconds
	if (Saving) PutText(m_pd3dDevice,0,100,saving_text_buffer);

	sprintf(text_buffer , "FPS:  %f\n" , 
		m_pHostDemo->m_fFPS  );
	PutText(m_pd3dDevice,0,0,text_buffer);

	sprintf(text_buffer , "Shots: %d" , shots);
	PutText(m_pd3dDevice,0,80,text_buffer);

	sprintf(text_buffer , "Shots: %d\nTime: %d/%d" , shots , m_dwTime , m_dwDuration);
	PutText(m_pd3dDevice,0,180,text_buffer);

	// Render Negative
	if (m_dwTime > NEGATIVE1_BEGIN && m_dwTime <NEGATIVE1_END)
	{
		m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ZERO);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVDESTCOLOR);
		RenderSpecificQuad(m_pd3dDevice ,0,0,1,1,0x00000000);
	}
	
	// Render Negative
	if (m_dwTime > NEGATIVE2_BEGIN && m_dwTime <NEGATIVE2_END)
	{
		m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ZERO);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVDESTCOLOR);
		RenderSpecificQuad(m_pd3dDevice ,0,0,1,1,0x00000000);
	}

	m_pd3dDevice->EndScene();
}

void Scene4::Release()
{

	Billboard.Release();

#ifdef CAMERA_RECORDING
	// Save the camera keyframes
	fwrite(&shots , sizeof(unsigned long) , 1 , output);
	fwrite(camera_buffer , sizeof(CameraShot) , shots , output);
	fclose(output);
#endif

	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);

	m_pd3dDevice = NULL;
}
