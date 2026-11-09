// SCenes.cpp


#include "GreetzScene.h"
#include <stdio.h>

// GLOBALS /////////////////////////////////////////////////

extern char text_buffer[];
extern DWORD frames;

extern TextureManager TexMan;

float Sync(unsigned long time , unsigned long cycle)
{

	unsigned long T = time % cycle;

	if (T <= cycle/4) return 0.01f;
	if (T > cycle/4 && T < cycle/2) return (T-cycle/4) / ((float) cycle / 4.0f);
	if (T >= cycle/2 && T < 3*cycle/4) return 1;
	if (T >= 3*cycle/4) return 1.0f - ((float)(T-(3*cycle/4))/(float) (cycle/4.0f));
}

/////////////////////////////////////////////////////////////

bool Scene2::Init()
{

	m_pd3dDevice = m_pHostDemo->m_pd3dDevice;

	// Debug
	WriteToLog("<b>Greetings Scene</b><hr><ul>");

	if (!m1.Create(40,40,40,
		200,200,200,6))
	{
		WriteToLog("<li>IsoSurface - Failed to init</li>");
	}
	else
	{
		WriteToLog("<li>IsoSurface - OK</li>");
	}
	if (!m1.CreateBuffers(m_pd3dDevice))
	{
		WriteToLog("<li>Failed to create a vertex buffer for the IsoSurface</li>");
		return 0;
	}
	else
	{
		WriteToLog("IsoSurface - VertexBuffer - OK");
	}

	m1.pBalls[0].x = -15;
	m1.pBalls[0].y = 0;
	m1.pBalls[0].z = 0;
	m1.pBalls[0].mass = 150;
	
	m1.pBalls[1].x = 15;
	m1.pBalls[1].y = 0;
	m1.pBalls[1].z = 0;
	m1.pBalls[1].mass = 50;

	m1.pBalls[2].x = 0;
	m1.pBalls[2].y = 20;
	m1.pBalls[2].z = 0;
	m1.pBalls[2].mass = 50;

	m1.pBalls[3].x = -25;
	m1.pBalls[3].y = -10;
	m1.pBalls[3].z = -20;
	m1.pBalls[3].mass = 50;

	m1.pBalls[4].x = -25;
	m1.pBalls[4].y = -10;
	m1.pBalls[4].z = -20;
	m1.pBalls[4].mass = 90;

	m1.pBalls[5].x = -25;
	m1.pBalls[5].y = -10;
	m1.pBalls[5].z = -20;
	m1.pBalls[5].mass = 90;

	m1.IsoLevel = 1.0f;
	m1.Update();
	m1.RefreshBuffers();

	
	if (!ma1.LoadFrom3DS("data\\geometry\\Eggplant.3ds" , "PlastiKPurple01"))
	{
		WriteToLog("<li>Could NOT load the material - PlastiKPurple from eggplant.3ds</li>");
		return false;
	}
	else
	{
		WriteToLog("<li>Material PlastiKPurple01 from Eggplants.3ds - loaded OK</li>");
	}

	ma1.Data.Power = 50.0f;

	camera1.Set(0,0,-75,0,0,0);

	// Create the cubetexture
	if (FAILED(D3DXCreateCubeTextureFromFile(          
			m_pd3dDevice,
			"data\\textures\\chrome.dds",
			&CubeMap
			)))
	{
		WriteToLog("<li>Couldnt load the chrome.dds Cubetexture</li>");
	}
	else
	{
		WriteToLog("<li>Chrome.dds cubetexture - OK</li>");
	}

	m2.CreateRectangle(2,2,50,250);
	m2.CreateBuffers(m_pd3dDevice);
	m2.ApplyColor(0xffffffff);
	m2.RefreshBuffers();

	Asd.CreateRectangle(2,2,64,32);
	Asd.CreateBuffers(m_pd3dDevice);
	Asd.ApplyColor(0xffffffff);
	Asd.RefreshBuffers();

	WriteToLog("<li>Grids - OK</li>");

	// Create a texture to be used as a render target
	if (FAILED(m_pd3dDevice->CreateTexture(256,256,1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&tex1,
		NULL)))
	{
		WriteToLog("<li>Render texture1 could not be created!</li>");
		return 0;
	}
	else
	{
		WriteToLog("<li>Render target texture1 - OK</li>");
	}



	if (FAILED(m_pd3dDevice->CreateTexture(256,256,1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&tex2,
		NULL)))
	{
		WriteToLog("<li>Render texture2 could not be created!</li>");
	}
	else
	{
		WriteToLog("<li>Render target texture2 - OK</li>");
	}

	clean_tex = tex1;
	scratch_tex = tex2;


	m3.Create(350.0f);
	m3.CreateBuffers(m_pd3dDevice);
	m3.ApplyColor(0xefefefef);
	m3.RefreshBuffers();

	WriteToLog("<li>CubeRoom - OK</li>");

	// Set bilinear filtering for all textures
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MINFILTER , D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0 , D3DSAMP_MAGFILTER , D3DTEXF_LINEAR);

	// clean both textures before we begin
	m_pd3dDevice->BeginScene();

		RenderToTexture(scratch_tex , m_pd3dDevice);

			m_pd3dDevice->Clear(0,NULL,
			D3DCLEAR_TARGET ,
			D3DCOLOR_ARGB(255,0,0,0),1.0f,0);

		EndRenderToTexture(m_pd3dDevice);

		RenderToTexture(clean_tex , m_pd3dDevice);

			m_pd3dDevice->Clear(0,NULL,
			D3DCLEAR_TARGET ,
			D3DCOLOR_ARGB(255,0,0,0),1.0f,0);

		EndRenderToTexture(m_pd3dDevice);

	m_pd3dDevice->EndScene();

	// load textures
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/Asd.png");
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/Conspiracy.png");
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/Deus.png");
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/MadWizards.png");
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/TheLab.png");
	TexMan.LoadTexture(m_pd3dDevice , "Greetz/SenseAmok.png");

	// Debug
	WriteToLog("</ul>");

	return true;

}

void Scene2::Render()
{

m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE , FALSE);

	// Set Modulation
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLORARG2, D3DTA_TEXTURE);
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLOROP, D3DTOP_MODULATE);
	
/*
	// Swap the textures
	swap = clean_tex;
	clean_tex = scratch_tex;
	scratch_tex = swap;
*/
	// First render to a texture
	RenderToTexture(scratch_tex , m_pd3dDevice);

		//********************************
		// MATRIX STUFF
		//********************************

		D3DXMATRIXA16 matWorld,matWorld2,matWorld3;
		D3DXMatrixIdentity(&matWorld3);
		// World Matrix
		D3DXMatrixRotationY( &matWorld, ((1.0f+(float)sin(m_dwTime/1000.0f)))*D3DX_PI );
		D3DXMatrixRotationX( &matWorld2, ((1.0f+(float)sin(m_dwTime/684.0f)))*D3DX_PI );
		D3DXMatrixMultiply(&matWorld3,&matWorld,&matWorld2);
		m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld3 );

		
		// View Matrix
		//camera1.UpdateViewMatrix(m_pd3dDevice);
		D3DXVECTOR3 up,to,pos;
		D3DXMATRIXA16 matView;
		pos.x = 100 * sin((float)m_dwTime/1390.0f);
		pos.y = 0;//200 * cos((float)m_dwTime/390.0f);
		pos.z = 100 * cos((float)m_dwTime/1390.0f);
		up.x=100 * cos((float)m_dwTime/3390.0f);;up.y=1;up.z=100 * sin((float)m_dwTime/1390.0f);;
		to.x=0;to.y=0;to.z=0;

		D3DXMatrixLookAtLH(&matView,&pos,&to,&up);
		
		m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

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
		D3DLIGHT9 light , light2;
		ZeroMemory( &light, sizeof(D3DLIGHT9) );
		light.Type       = D3DLIGHT_DIRECTIONAL;
		light.Diffuse.r  = 1.9f;
		light.Diffuse.g  = 0.9f;
		light.Diffuse.b  = 1.9f;
		light.Specular.r  = 0.6f;
		light.Specular.g  = 0.6f;
		light.Specular.b  = 0.9f;
		vecDir = D3DXVECTOR3(0,
							 -0.3f,
							 1.0f );
		D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &vecDir );
		light.Range       = 1000.0f;
		m_pd3dDevice->SetLight( 0, &light );
		m_pd3dDevice->LightEnable( 0, TRUE );

		ZeroMemory( &light2, sizeof(D3DLIGHT9) );
		light2.Type       = D3DLIGHT_DIRECTIONAL;
		light2.Diffuse.r  = 0.9f;
		light2.Diffuse.g  = 0.9f;
		light2.Diffuse.b  = 0.9f;
		light2.Specular.r  = 0.7f;
		light2.Specular.g  = 0.7f;
		light2.Specular.b  = 0.4f;
		vecDir = D3DXVECTOR3(0,
							 0.3f,
							 -1.0f );
		D3DXVec3Normalize( (D3DXVECTOR3*)&light2.Direction, &vecDir );
		light2.Range       = 1000.0f;
		m_pd3dDevice->SetLight( 1, &light2 );
		m_pd3dDevice->LightEnable( 1, TRUE );

		m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

		// Finally, turn on some ambient light.
		m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00202020 );
		
		// Enable specular
		m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_LOCALVIEWER , TRUE);
		
		// Turn on Z
		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);


		m_pd3dDevice->Clear(0,NULL,
			/*D3DCLEAR_TARGET |*/ D3DCLEAR_ZBUFFER,
			D3DCOLOR_ARGB(255,0,0,0),1.0f,0);

		// Normalize normals
		m_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS,TRUE);

		// Set the cubemap
		m_pd3dDevice->SetTexture(0 , CubeMap);

		// Autocalculate cubemap coords
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
			 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );

		// Render the mesh
		m_pd3dDevice->SetMaterial(&ma1.Data);
		m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		m1.pBalls[0].x = 19 * sin(m_dwTime/600.0f);
		m1.pBalls[1].y = 18 * sin(m_dwTime/530.0f);
		m1.pBalls[2].z = 18 * sin(m_dwTime/730.0f);
		m1.pBalls[3].x = 28 * sin(m_dwTime/230.0f);
		m1.pBalls[3].y = 18 * sin(m_dwTime/460.0f);
		m1.pBalls[3].z = 15 * sin(m_dwTime/730.0f);
		m1.pBalls[4].x = 13 * sin(m_dwTime/230.0f);
		m1.pBalls[4].y = 17 * sin(m_dwTime/270.0f);
		m1.pBalls[4].z = 15 * sin(m_dwTime/290.0f);
		m1.pBalls[5].x = 11 * sin(m_dwTime/530.0f);
		m1.pBalls[5].y = 13 * sin(m_dwTime/570.0f);
		m1.pBalls[5].z = 12 * sin(m_dwTime/290.0f);
		bool upd = false;
		if (m_pHostDemo->m_fFPS > 30)
		{
			m1.Update();
			m1.RefreshBuffers();
		}

		m1.Render(m_pd3dDevice);

		m_pd3dDevice->SetTexture(0,clean_tex);
/*
		if (static bool firsttime = true)
		{
			m_pd3dDevice->SetTexture(0,0);
			firsttime = false;
		}
*/
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
			 0 );
		m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
		D3DXMatrixIdentity(&matWorld3);
		m_pd3dDevice->SetTransform(D3DTS_WORLD , &matWorld3);


		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCCOLOR);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCCOLOR);

		m3.Render(m_pd3dDevice);

		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);

	// Finished rendering to texture
	EndRenderToTexture(m_pd3dDevice);

	// Swap the textures
	swap = clean_tex;
	clean_tex = scratch_tex;
	scratch_tex = swap;

	// Render again

	//********************************
	// MATRIX STUFF
	//********************************

	D3DXMatrixIdentity(&matWorld3);
	// World Matrix
	D3DXMatrixRotationY( &matWorld, ((1.0f+(float)sin(m_dwTime/1000.0f)))*D3DX_PI );
	D3DXMatrixRotationX( &matWorld2, ((1.0f+(float)sin(m_dwTime/684.0f)))*D3DX_PI );
	D3DXMatrixMultiply(&matWorld3,&matWorld,&matWorld2);
	m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld3 );

	
	// View Matrix
	//camera1.UpdateViewMatrix(m_pd3dDevice);
		pos.x = 100 * sin((float)m_dwTime/1390.0f);
		pos.y = 0;//200 * cos((float)m_dwTime/390.0f);
		pos.z = 100 * cos((float)m_dwTime/1390.0f);
		up.x=100 * cos((float)m_dwTime/3390.0f);;up.y=1;up.z=100 * sin((float)m_dwTime/1390.0f);;
		to.x=0;to.y=0;to.z=0;

	D3DXMatrixLookAtLH(&matView,&pos,&to,&up);
	
	m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	// Projection Matrix
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 4.0f/3.0f, 1.0f, 10000.0f );
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	//********************************
	// END MATRIX STUFF
	//********************************

	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

	// Finally, turn on some ambient light.
	m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00202020 );
	
	// Enable specular
	m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_LOCALVIEWER , TRUE);
	
	// Turn on Z
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);



	m_pd3dDevice->Clear(0,NULL,
		/*D3DCLEAR_TARGET |*/ D3DCLEAR_ZBUFFER,
		D3DCOLOR_ARGB(255,255,255,255),1.0f,0);

	m_pd3dDevice->SetTexture(0,scratch_tex);
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 0 );
	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	D3DXMatrixIdentity(&matWorld3);
	m_pd3dDevice->SetTransform(D3DTS_WORLD , &matWorld3);

	//if ((m_pHostDemo->m_fFPS > 40) || (m_dwTime<1000))
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCCOLOR);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCCOLOR);

	m3.Render(m_pd3dDevice);

	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , FALSE);


	// Normalize normals
	m_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS,TRUE);

	// Set the cubemap
	m_pd3dDevice->SetTexture(0 , CubeMap);

	// Autocalculate cubemap coords
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
		 D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | 1 );

	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );


	// Render the mesh
	m_pd3dDevice->SetMaterial(&ma1.Data);

	m1.Render(m_pd3dDevice);



	pos.x = 150;
	pos.y = 0;
	pos.z = -300;
	up.x=0;up.y=-1;up.z=0;
	to.x=150;to.y=0;to.z=0;

	D3DXMatrixLookAtLH(&matView,&pos,&to,&up);
	m_pd3dDevice->SetTransform(D3DTS_VIEW,&matView);
m_pd3dDevice->SetRenderState(D3DRS_ZENABLE , FALSE);
	TexMan.SetTexture(m_pd3dDevice , 0 , "dna.png");
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE , TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 
			 0 );

	m2.ClearTransform();
	m2.Scroll(0,m_dwTime/1000.0f);
	m2.Render(m_pd3dDevice);

	pos.x = -150;
	pos.y = 0;
	pos.z = -300;
	up.x=0;up.y=1;up.z=0;
	to.x=-150;to.y=0;to.z=0;
		D3DXMatrixLookAtLH(&matView,&pos,&to,&up);
		m_pd3dDevice->SetTransform(D3DTS_VIEW,&matView);

		m2.Render(m_pd3dDevice);

	pos.x = 0;
	pos.y = 0;
	pos.z = -300;
	up.x=0;up.y=1;up.z=0;
	to.x=-0;to.y=0;to.z=0;
		D3DXMatrixLookAtLH(&matView,&pos,&to,&up);
		m_pd3dDevice->SetTransform(D3DTS_VIEW,&matView);

	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_ONE);



	float par ;

	par = Sync(m_dwTime , 4000);
	TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/Asd.png");
	Asd.ClearTransform();
	Asd.SetTransform(TRANSFORM_SCALE , 1.3*par*2.50f , 1.1*1.50f/par/4 , 3*1.0f);
	Asd.SetTransform(TRANSFORM_TRANSLATE , -110*par,110,0);
	Asd.Render(m_pd3dDevice);

	par = Sync(m_dwTime , 5000);
	TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/TheLab.png");
	Asd.ClearTransform();
	Asd.SetTransform(TRANSFORM_SCALE , 1.1f*par*2.50f , 3*1.50f/par/4 , 3*1.0f);
	Asd.SetTransform(TRANSFORM_TRANSLATE , -130*par+30,80,0);
	Asd.Render(m_pd3dDevice);

	par = Sync(m_dwTime , 3000);
	TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/deus.png");
	Asd.ClearTransform();
	Asd.SetTransform(TRANSFORM_SCALE , 1.1f*par*2.50f , 1.1f*1.50f/par/3 , 3*1.0f);
	Asd.SetTransform(TRANSFORM_TRANSLATE , 110*par-30,110,0);
	Asd.Render(m_pd3dDevice);

	par = Sync(m_dwTime , 6000);
	if (m_dwTime < 6000)
		TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/Madwizards.png");
	else
		TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/Conspiracy.png");
	Asd.ClearTransform();
	Asd.SetTransform(TRANSFORM_SCALE , 1.1f*par*2.50f , 2.1f*1.50f/par/3 , 3*1.0f);
	Asd.SetTransform(TRANSFORM_TRANSLATE , -170*par+100,-110,0);
	if (m_dwTime < 12000)
		Asd.Render(m_pd3dDevice);

	par = Sync(m_dwTime , 7000);
	TexMan.SetTexture(m_pd3dDevice , 0 , "Greetz/SenseAmok.png");
	Asd.ClearTransform();
	Asd.SetTransform(TRANSFORM_SCALE , 1.1f*par*1.50f , 1.1f*1.50f/par/3 , 3*1.0f);
	Asd.SetTransform(TRANSFORM_TRANSLATE , 20*par+100,-70,0);
	Asd.Render(m_pd3dDevice);

	sprintf(text_buffer , "FPS:  %f" , 
		m_pHostDemo->m_fFPS);
	PutText(m_pd3dDevice,0,0,text_buffer);

	// Present the sin!
	m_pd3dDevice->Present(NULL,NULL,NULL,NULL);
	

}

void Scene2::Release()
{

	m1.Release();
	m2.Release();

	Asd.Release();

	if (tex1)
		tex1->Release();
	if (tex2)
		tex2->Release();

	if (CubeMap)
		CubeMap->Release();

	m_pd3dDevice = NULL;
}
