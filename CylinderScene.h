// Scenes.h
// This just holds information about the scene classes

// LOCK ///////////////////////////////////////////

// INCLUDES ///////////////////////////////////////

#include "DemoEngine.h"

// CLASS DECLARATIONS /////////////////////////////



class Scene3 : public Scene
{
public:

	Mesh3DS Cylinder , Cylinder2;
	MeshBillboard Particles;
	Material m1;
	MeshGrid m2 , Names;
	LPDIRECT3DCUBETEXTURE9 CubeMap;
	virtual bool Init();
	virtual void Render();
	virtual void Release();

	CameraShot camera1;
	

}; // End class Scene1




// LOCK ///////////////////////////////////////////