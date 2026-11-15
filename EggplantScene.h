// Scenes.h
// This just holds information about the scene classes

// LOCK ///////////////////////////////////////////

// INCLUDES ///////////////////////////////////////

#include "DemoEngine.h"

#include "camera.h"

// CLASS DECLARATIONS /////////////////////////////



class Scene1 : public Scene
{
public:
	/*
	Mesh3DS m1;
	MeshGrid m2,m4;
	MeshInCube m3;
	Material* ma1;
	MaterialManager MaMa;*/

	MeshBillboard  light_source;

	Scene3DS scene3d,floor1,granazi;
	Mesh3DS *pStem,*pEgg;
	Material *pMatStem,*pMatEgg;
	LPDIRECT3DCUBETEXTURE9 CubeMap;
	LPDIRECT3DTEXTURE9	asd_tex,tex1,tex2,clean_tex,scratch_tex,swap;

	virtual bool Init();
	virtual void Render();
	virtual void Release();

	CameraShot camera1;
	CameraSequence Seq1;

	FILE* output;
	bool  Saving;
	char saving_text_buffer[1256];
	CameraShot camera_buffer[1000];
	unsigned long shots;
	

}; // End class Scene1




// LOCK ///////////////////////////////////////////