// TextureManager.cpp
// see also :  TextureManager.h

#include "TextureManager.h"
#include "log.h"

TextureEntry::TextureEntry()
{
	NextEntry = NULL;
	pTex = NULL;
	name[0] = 0;
	Valid = false;
}

TextureEntry::~TextureEntry()
{
	Release();
}

void TextureEntry::Release()
{
	if (pTex)
		pTex->Release();
}

int TextureEntry::Load(LPDIRECT3DDEVICE9 pDevice ,const char* filename)
{
	// return zero if no name
	if (!filename[0]) return 0;

	// load the correct name
	int x=0;
	while (filename[x])
	{
		name[x] = filename[x];
		x++;
	}
	name[x] = 0;

	char file[40];
	const char* prefix = "data\\textures\\";

	x=0;
	while (prefix[x])
	{
		file[x] = prefix[x];
		x++;
	}

	int y=0;
	while (filename[y])
	{
		file[x] = filename[y];
		x++; y++;
	}

	file[x]=0;

	if (FAILED(D3DXCreateTextureFromFileEx(pDevice,
							file,
							0,0,
							D3DX_DEFAULT,
							0,
							D3DFMT_DXT1,
							D3DPOOL_DEFAULT,
							D3DX_DEFAULT ,
							D3DX_DEFAULT ,
							0,
							NULL,
							NULL,
							&pTex)))
	{
		WriteToLog("Unable to load the texture:<br> ");
		WriteToLog(file);
		return 0;
	}

	Valid = true;
	return 1;
}

void TextureEntry::Set(LPDIRECT3DDEVICE9 pDevice , 
					   unsigned long sampler)
{
	if (Valid) pDevice->SetTexture(sampler , pTex);
	else pDevice->SetTexture(sampler , 0);
}

bool TextureEntry::IsTheName(const char* n)
{

	int x=0;
	while (name[x]!=0 && n[x]!=0)
	{
		if (name[x]!=n[x]) return false;
		x++;
	}

	if (name[x]) return false;
	if (n[x]) return false;

	return true;
}

// TEXTURE MANAGER //////////

TextureManager::TextureManager()
{
	pFirstEntry = NULL;
}

void TextureManager::Release()
{
	TextureEntry *entry,*temp;
	entry = pFirstEntry;

	while (entry)
	{
		temp = entry->NextEntry;
		delete entry;
		entry = temp;
	}
}

TextureEntry* TextureManager::FindEntry(const char* n)
{
	TextureEntry *entry;
	entry = pFirstEntry;

	while (entry)
	{
		if (entry->IsTheName(n)) return entry;
		entry = entry->NextEntry;
	}

	return 0;
}


int	TextureManager::LoadTexture(LPDIRECT3DDEVICE9 pDevice , 
								const char* filename)
{
	if (pFirstEntry)
	{
		// First check if the texture already exists
		if (FindEntry(filename)) return 1;

		// else we have to load it from disk
		TextureEntry *new_tex = new TextureEntry();
		new_tex->Load(pDevice , filename);

		// A hack , to make sure that nextentry is always null
		// I don't know why , but sometimes it can be not zero...
		new_tex->NextEntry = NULL;

		// Add it as the last item of the linked list
		TextureEntry *temp = pFirstEntry;

		while (temp->NextEntry)
		{
			temp = temp->NextEntry;
		}

		temp->NextEntry = new_tex;

	}
	else
	{
		pFirstEntry = new TextureEntry();
		pFirstEntry->Load(pDevice , filename);
	}


	return 1;
}


void TextureManager::DeleteTexture(const char* n)
{
	// first try to locate the entry
	TextureEntry* entry = FindEntry(n);

	// if it does not exist , just return
	if (!entry) return;

	// link the previews entry to the next
	if (entry == pFirstEntry) pFirstEntry = pFirstEntry->NextEntry;

	else 
	{
		TextureEntry* temp = pFirstEntry;
		while (temp->NextEntry != entry)
		{
			temp = temp->NextEntry;
		}
		// Link to the next entry
		temp->NextEntry = temp->NextEntry->NextEntry;
	}

	// safely delete the texture entry
	delete entry;
	 
}


void TextureManager::SetTexture(LPDIRECT3DDEVICE9 pDevice , 
							unsigned long sampler,
							const char* n)
{
	if (!n)
	{
		pDevice->SetTexture(sampler , 0);
		return;
	}

	TextureEntry *tex;
	tex = FindEntry(n);
	if (tex) tex->Set(pDevice , sampler);
	else 
	{
		LoadTexture(pDevice , n);
		FindEntry(n)->Set(pDevice , sampler);;
	}

}


void TextureManager::SetNoTexture(LPDIRECT3DDEVICE9 pDevice)
{
	pDevice->SetTexture(0,0);
}