// SCenes.cpp


#include "EggplantScene.h"
#include <stdio.h>

// GLOBALS /////////////////////////////////////////////////

extern char text_buffer[];
extern DWORD frames;

FILE* output;
bool  Saving=0;
char saving_text_buffer[1256];
CameraShot camera_buffer[1000];
unsigned long shots = 0;

extern TextureManager TexMan;

//#define CAMERA_RECORDING

/////////////////////////////////////////////////////////////

bool Scene1::Init()
{
	
	// A better pointer to the device (less redirection)
	m_pd3dDevice = m_pHostDemo->m_pd3dDevice;

	// Debug
	WriteToLog("<b>Eggplants Scene</b><hr><ul>");


	if (!scene3d.LoadFromS3D("data\\geometry\\test.s3d",m_pd3dDevice))
	{
		WriteToLog("<li>Model test.s3d (eggplant) Failed to load</li>");
	}
	else
	{
		WriteToLog("<li>Model test.s3d (eggplant) - Loaded OK</li>");
	}

	// Create the cubetexture
	if (FAILED(D3DXCreateCubeTextureFromFile(          
			m_pd3dDevice,
			"data\\textures\\metal.dds",
			&CubeMap
			)))
	{
		WriteToLog("<li>Failed to load Metal.dds cubetexture</li>");
	}
	else
	{
		WriteToLog("<li> Cubetexture Metal.dds - OK</li>");
	}

	// Set the eggplant for external texture
	pStem = scene3d.GetObject("Stem");
	pEgg = scene3d.GetObject("Egg");
	pMatStem = scene3d.MaMa.GetMaterial(pStem->MaterialName);
	pMatEgg  = scene3d.MaMa.GetMaterial( pEgg->MaterialName);
	pStem->ExternalTexture = CubeMap;
	pEgg->ExternalTexture  = CubeMap;

	if (!floor1.LoadFromS3D("data\\geometry\\c_room.s3d",m_pd3dDevice))
	{
		WriteToLog("<li>Model c_room.s3d (circuit room) Failed to load</li>");
	}
	else
	{
		WriteToLog("<li>Model c_room.s3d (circuit room) - OK</li>");
	}

	// Init the billboard
	if (!light_source.Init(m_pd3dDevice , 1))
	{
		WriteToLog("<li>Couldn't initialize the billboard (lightsource)</li>");
	}
	else
	{
		WriteToLog("<li>Lightsource billboard - OK</li>");
	}
	light_source.ParticleList[0].x = 1000;
	light_source.ParticleList[0].y = 300;
	light_source.ParticleList[0].z = 0;
	light_source.ParticleList[0].color = 0xffff0000;
	light_source.ParticleList[0].size = 10;
	light_source.ParticleList[0].alive = true;

	// ** Camera Interface **
#ifdef CAMERA_RECORDING
	// camera
	camera1.Set(890,290,-80,D3DX_PI/2.7f,0,0.0f);
	// open the output file
	output = fopen("data/geometry/plants.cam" , "w+b");
#else
	// ** Camera Replay **
	if (!Seq1.Load("data/geometry/plants.cam"))
	{
		WriteToLog("<li>Failed to laod plants.cam</li>");
	}
	else
	{
		WriteToLog("<li>Camera Sequence - plants.cam - OK</li>");
	}
#endif


	// Debug
	WriteToLog("</ul>");

	return true;

}

#define EGG_DISTANCE	600.0f
#define	NUM_ROWS		3
#define EGG_SHIFT		EGG_DISTANCE*NUM_ROWS/2

void Scene1::Render()
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
	Seq1.Cerp(m_dwTime);
	Seq1.UpdateViewMatrix(m_pd3dDevice);

#endif
	// ** End Camera Interface **



	// Projection Matrix
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 4.0f/3.0f, 1.0f, 10000.0f );
	extra::applyEggplantCamera(*m_pd3dDevice, m_dwTime, matProj);
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
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);


	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(255,0,0,0),1.0f,0);

	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MIPFILTER , D3DTEXF_LINEAR);
	
	// Compute the viewing frustum
	CULLINFO frustum;
	D3DXMATRIXA16 ViewMatrix;
	m_pd3dDevice->GetTransform(D3DTS_VIEW , &ViewMatrix);
	UpdateCullInfo(&frustum , &ViewMatrix,&matProj);

	// Render the mesh
	unsigned long objects_clipped = 0;

	m_pd3dDevice->BeginScene();

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);


	// Autocalculate cubemap coords
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );

	// Set the stream source
	m_pd3dDevice->SetStreamSource(0 , pStem->pVB , 0 , sizeof(MODEL_VERTEX));
	// Set the fvf
	m_pd3dDevice->SetFVF(FVF_MODEL);

	// Set the index buffer
	m_pd3dDevice->SetIndices( pStem->pIB );

	// Set material
	pMatStem->Set(m_pd3dDevice , CubeMap);

	// Render the reflections
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pStem->ClearTransform();
			pStem->SetTransform(TRANSFORM_SCALE , 1,-1,1);
			pStem->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pStem->Transform);
			pStem->TransformBoundingBox();
			pStem->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pStem->BoundingBoxScratch,
				pStem->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pStem->NumVertices,
					0,
					pStem->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	// Render the reflections
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pStem->ClearTransform();
			pStem->SetTransform(TRANSFORM_SCALE , 1,-1,1);
			pStem->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT+EGG_DISTANCE/2, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT+EGG_DISTANCE/2);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pStem->Transform);
			pStem->TransformBoundingBox();
			pStem->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pStem->BoundingBoxScratch,
				pStem->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pStem->NumVertices,
					0,
					pStem->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	pMatEgg->Set(m_pd3dDevice , CubeMap);
	// Set the stream source
	m_pd3dDevice->SetStreamSource(0 , pEgg->pVB , 0 , sizeof(MODEL_VERTEX));
	// Set the index buffer
	m_pd3dDevice->SetIndices( pEgg->pIB );
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pEgg->ClearTransform();
			pEgg->SetTransform(TRANSFORM_SCALE , 1,-1,1);
			pEgg->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pEgg->Transform);
			pEgg->TransformBoundingBox();
			pEgg->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pEgg->BoundingBoxScratch,
				pEgg->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pEgg->NumVertices,
					0,
					pEgg->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pEgg->ClearTransform();
			pEgg->SetTransform(TRANSFORM_SCALE , 1,-1,1);
			pEgg->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT+EGG_DISTANCE/2, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT+EGG_DISTANCE/2);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pEgg->Transform);
			pEgg->TransformBoundingBox();
			pEgg->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pEgg->BoundingBoxScratch,
				pEgg->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pEgg->NumVertices,
					0,
					pEgg->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	// original texcoords
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_PASSTHRU );

	// Now render the reflective floor
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);
	floor1.ClearTransform();
	floor1.SetTransform(TRANSFORM_SCALE , 400,400,400);
	floor1.SetTransform(TRANSFORM_TRANSLATE , 0,50,0);
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_CW);
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);
	floor1.BatchRender(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_CCW);

	// Set the cubemap
	m_pd3dDevice->SetTexture(0 , CubeMap);

	// Autocalculate cubemap coords
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	// Set the stream source
	m_pd3dDevice->SetStreamSource(0 , pStem->pVB , 0 , sizeof(MODEL_VERTEX));
	pMatStem->Set(m_pd3dDevice , CubeMap);
	// Set the index buffer
	m_pd3dDevice->SetIndices( pStem->pIB );
	// Render the eggplants
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pStem->ClearTransform();
			pStem->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pStem->Transform);
			pStem->TransformBoundingBox();
			pStem->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pStem->BoundingBoxScratch,
				pStem->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pStem->NumVertices,
					0,
					pStem->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	// Render the eggplants
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pStem->ClearTransform();
			pStem->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT+EGG_DISTANCE/2, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT+EGG_DISTANCE/2);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pStem->Transform);
			pStem->TransformBoundingBox();
			pStem->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pStem->BoundingBoxScratch,
				pStem->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pStem->NumVertices,
					0,
					pStem->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	// Set the stream source
	m_pd3dDevice->SetStreamSource(0 , pEgg->pVB , 0 , sizeof(MODEL_VERTEX));
	pMatEgg->Set(m_pd3dDevice , CubeMap);
	// Set the index buffer
	m_pd3dDevice->SetIndices( pEgg->pIB );
	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pEgg->ClearTransform();
			pEgg->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pEgg->Transform);
			pEgg->TransformBoundingBox();
			pEgg->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pEgg->BoundingBoxScratch,
				pEgg->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pEgg->NumVertices,
					0,
					pEgg->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	for (unsigned long index_x=0;index_x<NUM_ROWS;index_x++)
	{
		for (unsigned long index_y=0;index_y<NUM_ROWS;index_y++)
		{
			pEgg->ClearTransform();
			pEgg->SetTransform(TRANSFORM_TRANSLATE , 
				EGG_DISTANCE*index_x-EGG_SHIFT+EGG_DISTANCE/2, 0 , 
				EGG_DISTANCE*index_y-EGG_SHIFT+EGG_DISTANCE/2);

			// Transform object and bounding box
			m_pd3dDevice->SetTransform(D3DTS_WORLD , &pEgg->Transform);
			pEgg->TransformBoundingBox();
			pEgg->CalculateBBPlanes();

			if (ObjectInFrustum(&frustum , pEgg->BoundingBoxScratch,
				pEgg->BBPlanes))
			{
				// Render the damn mesh!
				m_pd3dDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST , 
					0 , 
					0,
					pEgg->NumVertices,
					0,
					pEgg->NumIndices/3);
			}
			else
			{
				objects_clipped++;
			}
		}
	}

	// original texcoords
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_PASSTHRU );

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_CW);


	// Debug output
	// if we are saving print the number of miliseconds
	if (Saving) PutText(m_pd3dDevice,0,100,saving_text_buffer);

	sprintf(text_buffer , "FPS:  %f\nObjects Clipped: %d" , 
		m_pHostDemo->m_fFPS , objects_clipped );
	PutText(m_pd3dDevice,0,0,text_buffer);

	sprintf(text_buffer , "Shots: %d\nTime: %d/%d" , shots , m_dwTime , m_dwDuration);
	PutText(m_pd3dDevice,0,80,text_buffer);


	m_pd3dDevice->EndScene();

	// render Light and flares
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING , FALSE);

	D3DXMATRIXA16 iden;
	D3DXMatrixIdentity(&iden);
	m_pd3dDevice->SetTransform(D3DTS_WORLD , &iden);

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE , D3DCULL_NONE);


	// render tester in red
	TexMan.SetTexture(m_pd3dDevice , 0 , 0);
	light_source.ClearTransform();
	light_source.ParticleList[0].size = 8;
	light_source.ParticleList[0].color = 0xffff0000;
	light_source.Render(m_pd3dDevice);

	// Get the color at the light's position
	unsigned long pix = GetColorAtPosition(m_pd3dDevice , 
		light_source.ParticleList[0].x, 
		light_source.ParticleList[0].y , 
		light_source.ParticleList[0].z);

	// if it is red , then render the light
	if (pix == 0xffff0000)
	{
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);

		unsigned long flare_color = FlareIntensity(m_pd3dDevice ,
				light_source.ParticleList[0].x, 
				light_source.ParticleList[0].y , 
				light_source.ParticleList[0].z);

		TexMan.SetTexture(m_pd3dDevice , 0 , "star.png");
		light_source.ClearTransform();
		light_source.ParticleList[0].size = 1000;
		light_source.ParticleList[0].color = flare_color;
		light_source.Render(m_pd3dDevice);

		// make a line from the light to the center of the screen
		// full of flares
		int cx,cy,lx,ly,dx,dy;
		GetXY(m_pd3dDevice ,
				light_source.ParticleList[0].x, 
				light_source.ParticleList[0].y , 
				light_source.ParticleList[0].z,
				   &lx , &ly);
		GetScreenCenter(&cx, &cy);
		dx = cx-lx; dy = cy-ly;

		// Render first flare
		TexMan.SetTexture(m_pd3dDevice , 0 , "lens2.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+0.4f*dx , ly+0.4f*dy , 
					  0.05f,
					  flare_color);

		TexMan.SetTexture(m_pd3dDevice , 0 , "lens3.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+0.45f*dx , ly+0.45f*dy , 
					  0.15f,
					  flare_color);

		TexMan.SetTexture(m_pd3dDevice , 0 , "lens2.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+0.5f*dx , ly+0.5f*dy , 
					  0.05f,
					  flare_color);

		TexMan.SetTexture(m_pd3dDevice , 0 , "lens3.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+1.5f*dx , ly+1.5f*dy , 
					  0.07f,
					  flare_color);

		TexMan.SetTexture(m_pd3dDevice , 0 , "lens4.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+1.57f*dx , ly+1.57f*dy , 
					  0.06f,
					  flare_color);

		TexMan.SetTexture(m_pd3dDevice , 0 , "lens1.png");
		RenderQuadAt(m_pd3dDevice , 
					  lx+1.64f*dx , ly+1.64f*dy , 
					  0.08f,
					  flare_color);

		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);
	}


	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

	// Present the sin!
	m_pd3dDevice->Present(NULL,NULL,NULL,NULL);
	

}

void Scene1::Release()
{

#ifdef CAMERA_RECORDING
	// Save the camera keyframes
	fwrite(&shots , sizeof(unsigned long) , 1 , output);
	fwrite(camera_buffer , sizeof(CameraShot) , shots , output);
	fclose(output);
#endif


/*
	m_pd3dDevice->Clear(0,NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(0,0,0,0),1.0f,0);

*/
	scene3d.Release();
	floor1.Release();

	light_source.Release();

	//m1.Release();

	
	//m2.Release();
	//m3.Release();
	

	m_pd3dDevice = NULL;
}
