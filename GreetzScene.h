// GreetzScene.h
// This just holds information about the scene classes

// LOCK ///////////////////////////////////////////

// INCLUDES ///////////////////////////////////////

#include "DemoEngine.h"

// CLASS DECLARATIONS /////////////////////////////



class Scene2 : public Scene
{
public:

	MeshIso m1;
	Material ma1;
	MeshGrid m2,Asd,Conspiracy,deus,Madwizards,TheLab;
	MeshInCube m3;

	LPDIRECT3DCUBETEXTURE9 CubeMap;
	LPDIRECT3DTEXTURE9	tex1,tex2,clean_tex,scratch_tex,swap ;

	virtual bool Init();
	virtual void Render();
	virtual void Release();

	CameraShot camera1;
	

}; // End class Scene1



// LOCK ///////////////////////////////////////////
