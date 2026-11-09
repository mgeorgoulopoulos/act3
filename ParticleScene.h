// Scenes.h
// This just holds information about the scene classes

// LOCK ///////////////////////////////////////////

// INCLUDES ///////////////////////////////////////

#include "DemoEngine.h"

// CLASS DECLARATIONS /////////////////////////////



class Scene4 : public Scene
{
public:

	MeshBillboard Billboard,Star;

	Scene3DS shadow,room;
	Mesh3DS  *pRoom;
	Material *p_matRoom;

	bool Transition;
	
	virtual bool Init();
	virtual void Render();
	virtual void Release();

	void RenderEverything();

	CameraShot camera1;
	CameraSequence Seq1;

	// A render target texture
	LPDIRECT3DTEXTURE9 render_target;

	FILE* output;
	bool  Saving;
	char saving_text_buffer[256];
	CameraShot camera_buffer[1000];
	unsigned long shots;
	

}; // End class Scene1




// LOCK ///////////////////////////////////////////