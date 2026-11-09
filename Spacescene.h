// Scenes.h
// This just holds information about the scene classes

// LOCK ///////////////////////////////////////////

// INCLUDES ///////////////////////////////////////

#include "DemoEngine.h"

// CLASS DECLARATIONS /////////////////////////////



class SpaceScene : public Scene
{
public:

	Scene3DS Pioneer,Sympan,Man,Woman;
	MeshGrid m1;

	// stuff for the fire
	PDIRECT3DTEXTURE9 p_tex , p_empty_tex, tex_transition;
	unsigned char fire_buffer[256*256] , scratch_buffer[256*256];
	unsigned char* fire , *scratch ,*swap;
	DWORD palette[256];

	LPDIRECT3DCUBETEXTURE9 CubeMap,CubeMap2;
	virtual bool Init();
	virtual void Render();
	virtual void Release();

	void UpdateFire();
	void RenderFire();
	void RenderReversed();
	void RenderPatras();

	CameraShot camera1;
	CameraSequence Seq1;

	CameraShot camera_buffer[1000];

	FILE* output;
	bool  Saving;
	char saving_text[1256];
	unsigned long shots;
	

}; // End class Scene1




// LOCK ///////////////////////////////////////////