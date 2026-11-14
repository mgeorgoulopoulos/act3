// TextureManager.h
// A texture manager class acting as an abstraction layer
// between the device and the texture files on disk


// LOCK ////////////////////////////////////////////////

#ifndef __TEXTURE_MANAGER_HEADER
#define __TEXTURE_MANAGER_HEADER

// INCLUDES ////////////////////////////////////////////

#include "d3dx9.h"

// CLASSES /////////////////////////////////////////////

class TextureEntry
{
public:
	LPDIRECT3DTEXTURE9	pTex;
	char				name[100];
	TextureEntry*		NextEntry;
	bool				Valid;

	// Methods
	int			Load(LPDIRECT3DDEVICE9 pDevice ,char* filename);
	void		Release();
	void		Set(LPDIRECT3DDEVICE9 pDevice , unsigned long sampler);
	bool		IsTheName(char* n);

	// Constructor/ Destructor
	TextureEntry();
	~TextureEntry();

}; // End class TextureEntry

class TextureManager
{
public:

	TextureEntry*	pFirstEntry;

	// Methods
	TextureManager();
	void Release();

	int				LoadTexture(LPDIRECT3DDEVICE9 pDevice , 
								char* filename);
	void			DeleteTexture(char* n);
	void			SetTexture(LPDIRECT3DDEVICE9 pDevice , 
								unsigned long sampler,
								char* n);
	TextureEntry*	FindEntry(char* n);
	void SetNoTexture(LPDIRECT3DDEVICE9 pDevice);

}; // End class TextureManager

// LOCK ////////////////////////////////////////////////
#endif