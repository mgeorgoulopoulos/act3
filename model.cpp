// model.cpp
// methods for meshes and models

// INCLUDES //////////////////////////////////////////////////

#include "model.h"
#include "TextureManager.h"

// GLOBALS ///////////////////////////////////////////////////

// A large text buffer
char text_buffer1[1000] ;

extern TextureManager TexMan;

// FUNCTIONS /////////////////////////////////////////////////

// **********************************
// NeighborSurfaces
// **********************************

void NeighborSurfaces::Create(unsigned short n)
{

	Number = n;
	Buffer = (unsigned short*) malloc(n*sizeof(unsigned short));

}

void NeighborSurfaces::Release()
{

	free(Buffer);

}

void NeighborSurfaces::SetSurface(unsigned short n , unsigned short surface)
{

	if (!Buffer) return;
	if (n>=Number) return;

	Buffer[n] = surface;

}

// **********************************
// Mesh3DS
// **********************************

Mesh3DS::Mesh3DS()
{
	// GEOMETRY MEMORY
	VertexList			=NULL;
	IndexList			=NULL;
	NumVertices			=0;
	NumIndices			=0;

	// D3D BUFFERS
	pVB					=NULL;
	pIB					=NULL;

	// OPTIONAL GEOMETRY MEMORY
	pNeighbors			=NULL;
	pFaceNormals		=NULL;

	// 3ds stuff
	Name[0]				=0;
	MaterialName[0]		=0;

	// Transforms
	D3DXMatrixIdentity(&Transform);

	ExternalTexture		=NULL;

}

void Mesh3DS::Twist(float a , float t)
{
	float angle;

	// Fill the vb
	MODEL_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0,(void**) &pVertices , 0);

	// fill it up
	memcpy((void*)pVertices , VertexList , NumVertices * sizeof(MODEL_VERTEX));

	for (unsigned long i=0;i<NumVertices;i++)
	{
		angle = a*t*VertexList[i].z;

		pVertices[i].x = VertexList[i].x*cos(angle)-VertexList[i].y*sin(angle);
		pVertices[i].y = VertexList[i].x*sin(angle)+VertexList[i].y*cos(angle);
	}
	
	// Unlock VB
	pVB->Unlock();
}

void Mesh3DS::ClearTransform()
{
	D3DXMatrixIdentity(&Transform);
}

void Mesh3DS::SetTransform(TRANSFORM_TYPE t,
						   float par1,float par2,float par3)
{
	D3DXMATRIXA16 mat;
	switch (t)
	{
		case TRANSFORM_TRANSLATE:
			D3DXMatrixTranslation(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
		case TRANSFORM_ROTATE:
			D3DXMatrixRotationYawPitchRoll(&mat , par2,par1,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;

		case TRANSFORM_SCALE:
			D3DXMatrixScaling(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
	}
}

void Mesh3DS::TransformBoundingBox()
{
	for (int x=0;x<8;x++)
	{
		D3DXVec3TransformCoord(BoundingBoxScratch+x,
			BoundingBox+x,
			&Transform);
	}
}

int Mesh3DS::LoadFrom3DS(char* filename , char* object_name)
{

	FILE *input = fopen(filename,"rb");

	// See if the file is valid
	if (!input)
	{
		WriteToLog("No such file:");
		WriteToLog(filename);
		return 0;
	}

	// Check if this is a valid 3ds 
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , input);

	if (chunk_id != CHUNK_MAIN) 
	{
		WriteToLog("This is not a valid 3DS file:");
		WriteToLog(filename);
		fclose(input);
		return 0;
	}

	std::uint32_t chunk_length;
	fread(&chunk_length, sizeof(chunk_length) , 1, input);

	// Go to the 3d editor chunk
	if (!GoToChunk(CHUNK_3DEDITOR , input))
	{
		WriteToLog("This is not a valid 3DS file:");
		WriteToLog(filename);
		fclose(input);
		return 0;
	}

	// Enumerate all objects of the file
	while (1)
	{

		chunk_length = GoToChunk(CHUNK_OBJECTBLOCK , input);

		if (!chunk_length) 
		{
			WriteToLog("Not such object:");
			WriteToLog(object_name);
			WriteToLog("In File:");
			WriteToLog(filename);
			break;
		}

		// Read the objects name
		char name;
		int name_index = 0;
		while ((name = getc(input)))
		{
			text_buffer1[name_index] = name;
			name_index++;
			chunk_length--;
		}
		text_buffer1[name_index] = 0;
		chunk_length--;

		// DEBUG
		WriteToLog(text_buffer1);

		if (!strcmp(text_buffer1 , object_name))
		{
			// This is our object. Load it!
			// Go to the triangular mesh chunk
			if (!GoToChunk(CHUNK_TRIANGULARMESH , input))
				break;

			WriteToLog("<b>BLAAAAAAH!</b>");


			// Go to the vertices list chunk
			if (!GoToChunk(CHUNK_VERTICESLIST , input))
				break;

			// Load the vertices
			fread(&NumVertices , sizeof(unsigned short) , 1 , input);

			// Allocate space for the vertices
			VertexList = (MODEL_VERTEX*) malloc(NumVertices*sizeof(MODEL_VERTEX));


			// Copy the vertices
			for (unsigned int index = 0;index<NumVertices;index++)
			{
				float temp;
				fread(&temp , sizeof(float) , 1 , input);
				VertexList[index].x = temp;
				fread(&temp , sizeof(float) , 1 , input);
				VertexList[index].z = temp;
				fread(&temp , sizeof(float) , 1 , input);
				VertexList[index].y = temp;
			}

			// Go to the mapping coords chunk
			if (SearchForChunkUntil(CHUNK_MAPPINGCOORDS , 
									CHUNK_FACESDESCRIPTION ,
									input))
			{
				// Load the vertices
				fread(&NumVertices , sizeof(unsigned short) , 1 , input);

				// copy the mapping coords
				float uv;
				for (unsigned short uv_index = 0;uv_index<NumVertices;uv_index++)
				{
					fread(&uv , sizeof(float) , 1 , input);
					VertexList[uv_index].tu = uv;

					fread(&uv , sizeof(float) , 1 , input);
					VertexList[uv_index].tv = uv;
				}

				// Go to the faces description chunk
				if (!GoToChunk(CHUNK_FACESDESCRIPTION , input))
					break;

				// Get the polygon number
				unsigned short num_of_triangles;
				fread(&num_of_triangles , sizeof(unsigned short) , 1 , input);

				// Allocate memory for the triangles
				NumIndices = 3*num_of_triangles;
				IndexList = (unsigned short*) malloc(NumIndices*sizeof(unsigned short));

				// Copy the indices
				unsigned short temp_short;
				for (unsigned int index = 0;index<num_of_triangles;index++)
				{
					// Read the 3 vertices of the face
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index] = temp_short;
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index+2] = temp_short;
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index+1] = temp_short;

					// Read and discard face material
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
				}

				// Now read the material list chunk
				// only the first material for each 
				// object will be used
				if (!GoToChunk(CHUNK_MATERIALLIST , input))
				{
					WriteToLog("No material specified for this object:");
					WriteToLog(object_name);
					WriteToLog("In file:");
					WriteToLog(filename);
					break;
				}
				// copy the material's name
				int mat_name_count = 0;
				while (char matc = getc(input))
				{
					MaterialName[mat_name_count] = matc;
					mat_name_count++;
				}
				MaterialName[mat_name_count] = 0;
			}
			else
			{

				// We are already at the FACES DESCRIPTION CHUNK , or at the end of the file
				if (feof(input)) 
				{
					fclose(input);
					return 0;
				}

				// Get the polygon number
				unsigned short num_of_triangles;
				fread(&num_of_triangles , sizeof(unsigned short) , 1 , input);

				// Allocate memory for the triangles
				NumIndices = 3*num_of_triangles;
				IndexList = (unsigned short*) malloc(NumIndices*sizeof(unsigned short));

				// Copy the indices
				unsigned short temp_short;
				for (unsigned int index = 0;index<num_of_triangles;index++)
				{
					// Read the 3 vertices of the face
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index] = temp_short;
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index+2] = temp_short;
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
					IndexList[3*index+1] = temp_short;

					// Read and discard face material
					fread(&temp_short , sizeof(unsigned short) , 1 , input);
				}

				// Now read the material list chunk
				// only the first material for each 
				// object will be used
				if (!GoToChunk(CHUNK_MATERIALLIST , input))
				{
					WriteToLog("No material specified for this object:");
					WriteToLog(object_name);
					WriteToLog("In file:");
					WriteToLog(filename);
					break;
				}
				// copy the material's name
				int mat_name_count = 0;
				while (char matc = getc(input))
				{
					MaterialName[mat_name_count] = matc;
					mat_name_count++;
				}
				MaterialName[mat_name_count] = 0;
			}

			fclose(input);

			// Save the name
			strcpy(Name , object_name);

			// return success
			return 1;

		}
		else
		{
			// Skip this chunk
			//printf("Skipping object: %s" , text_buffer1);
			if (!SkipBytes(chunk_length-6,input)) 
			{
				fclose(input);
				return 0;
			}
		}


	} // End while


	// Close the file
	fclose(input);

	// return failure
	return 0;

}

void Mesh3DS::InvertNormals()
{

	for (unsigned long i=0;i<NumVertices;i++)
	{
		VertexList[i].nx = -VertexList[i].nx;
		VertexList[i].ny = -VertexList[i].ny;
		VertexList[i].nz = -VertexList[i].nz;
	}

}

bool Mesh3DS::RenderClipped(LPDIRECT3DDEVICE9 pDevice , 
							CULLINFO* frustum)
{
	// Transform the bounding box
	TransformBoundingBox();

	// Calculate the bbplanes
	CalculateBBPlanes();

	if (ObjectInFrustum(frustum , BoundingBoxScratch,BBPlanes)) 
	{
		Render(pDevice);
		return true;
	}

	return false;
}

bool Mesh3DS::BatchRenderClipped(LPDIRECT3DDEVICE9 pDevice , 
							CULLINFO* frustum)
{
	// Transform the bounding box
	TransformBoundingBox();

	// Calculate the bbplanes
	CalculateBBPlanes();

	if (ObjectInFrustum(frustum , BoundingBoxScratch,BBPlanes)) 
	{
		BatchRender(pDevice);
		return true;
	}

	return false;
}


void Mesh3DS::ComputeBoundingBox()
{
	// loop through all vertices and hold max/min values
	float xmax,ymax,zmax,xmin,ymin,zmin;
	xmax = VertexList[0].x;
	xmin = VertexList[0].x;

	ymax = VertexList[0].y;
	ymin = VertexList[0].y;

	zmax = VertexList[0].z;
	zmin = VertexList[0].z;

	for (unsigned long index=0;index<NumVertices;index++)
	{
		if (VertexList[index].x > xmax) xmax = VertexList[index].x;
		if (VertexList[index].x < xmin) xmin = VertexList[index].x;

		if (VertexList[index].y > ymax) ymax = VertexList[index].y;
		if (VertexList[index].y < ymin) ymin = VertexList[index].y;

		if (VertexList[index].z > zmax) zmax = VertexList[index].z;
		if (VertexList[index].z < zmin) zmin = VertexList[index].z;
	}

	// Now create the bb corners
	BoundingBox[0].x = xmin;
	BoundingBox[0].y = ymin;
	BoundingBox[0].z = zmin;

	BoundingBox[1].x = xmax;
	BoundingBox[1].y = ymin;
	BoundingBox[1].z = zmin;

	BoundingBox[2].x = xmax;
	BoundingBox[2].y = ymax;
	BoundingBox[2].z = zmin;

	BoundingBox[3].x = xmin;
	BoundingBox[3].y = ymax;
	BoundingBox[3].z = zmin;

	BoundingBox[4].x = xmin;
	BoundingBox[4].y = ymin;
	BoundingBox[4].z = zmax;

	BoundingBox[5].x = xmax;
	BoundingBox[5].y = ymin;
	BoundingBox[5].z = zmax;

	BoundingBox[6].x = xmax;
	BoundingBox[6].y = ymax;
	BoundingBox[6].z = zmax;

	BoundingBox[7].x = xmin;
	BoundingBox[7].y = ymax;
	BoundingBox[7].z = zmax;

	// Store the indices
	//    4+----+5
	//   /     /|
 	// 0+----+1 |
	//  |    |  |
	//  | 7+ |  +6
	// 3+----+2/

	BBIndices[0]  = 0;
	BBIndices[1]  = 1;
	BBIndices[2]  = 3;

	BBIndices[3]  = 1;
	BBIndices[4]  = 2;
	BBIndices[5]  = 3;

	BBIndices[6]  = 4;
	BBIndices[7]  = 5;
	BBIndices[8]  = 0;

	BBIndices[9]  = 5;
	BBIndices[10] = 1;
	BBIndices[11] = 0;

	BBIndices[12] = 1;
	BBIndices[13] = 5;
	BBIndices[14] = 2;

	BBIndices[15] = 5;
	BBIndices[16] = 6;
	BBIndices[17] = 2;

	BBIndices[18] = 4;
	BBIndices[19] = 0;
	BBIndices[20] = 3;

	BBIndices[21] = 7;
	BBIndices[22] = 4;
	BBIndices[23] = 3;

	BBIndices[24] = 5;
	BBIndices[25] = 4;
	BBIndices[26] = 7;

	BBIndices[27] = 6;
	BBIndices[28] = 5;
	BBIndices[29] = 7;

	BBIndices[30] = 3;
	BBIndices[31] = 6;
	BBIndices[32] = 7;

	BBIndices[33] = 2;
	BBIndices[34] = 6;
	BBIndices[35] = 3;
}

void Mesh3DS::SetBBIndices()
{
	// Store the indices
	//    4+----+5
	//   /     /|
 	// 0+----+1 |
	//  |    |  |
	//  | 7+ |  +6
	// 3+----+2/

	BBIndices[0]  = 0;
	BBIndices[1]  = 1;
	BBIndices[2]  = 3;

	BBIndices[3]  = 1;
	BBIndices[4]  = 2;
	BBIndices[5]  = 3;

	BBIndices[6]  = 4;
	BBIndices[7]  = 5;
	BBIndices[8]  = 0;

	BBIndices[9]  = 5;
	BBIndices[10] = 1;
	BBIndices[11] = 0;

	BBIndices[12] = 1;
	BBIndices[13] = 5;
	BBIndices[14] = 2;

	BBIndices[15] = 5;
	BBIndices[16] = 6;
	BBIndices[17] = 2;

	BBIndices[18] = 4;
	BBIndices[19] = 0;
	BBIndices[20] = 3;

	BBIndices[21] = 7;
	BBIndices[22] = 4;
	BBIndices[23] = 3;

	BBIndices[24] = 5;
	BBIndices[25] = 4;
	BBIndices[26] = 7;

	BBIndices[27] = 6;
	BBIndices[28] = 5;
	BBIndices[29] = 7;

	BBIndices[30] = 3;
	BBIndices[31] = 6;
	BBIndices[32] = 7;

	BBIndices[33] = 2;
	BBIndices[34] = 6;
	BBIndices[35] = 3;
}

void Mesh3DS::CalculateBBPlanes()
{
	// Calculate bb planes
	for (int index = 0;index<6;index++)
	{
		D3DXPlaneFromPoints(BBPlanes+index,
			&BoundingBoxScratch[BBIndices[6*index]],
			&BoundingBoxScratch[BBIndices[6*index+1]],
			&BoundingBoxScratch[BBIndices[6*index+2]]);
	}
}

bool Mesh3DS::IsTheName(char* n)
{

	int x=0;
	while (Name[x]!=0 && n[x]!=0)
	{
		if (Name[x]!=n[x]) return false;
		x++;
	}

	if (Name[x]) return false;
	if (n[x]) return false;

	return true;
}

int Mesh3DS::CreateBuffers(PDIRECT3DDEVICE9 pDevice)
{

	// Create a vertex buffer
	pDevice->CreateVertexBuffer(
		NumVertices*sizeof(MODEL_VERTEX),
		D3DUSAGE_WRITEONLY,
		FVF_MODEL,
		D3DPOOL_SYSTEMMEM,
		&pVB,
		NULL);

	// Create the index buffer
	pDevice->CreateIndexBuffer(
		NumIndices * sizeof(unsigned short),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_SYSTEMMEM,
		&pIB,
		NULL);

	return 1;

}

int Mesh3DS::RefreshBuffers()
{

	// Fill the vb
	void* pVertices;
	// Lock the vb
	pVB->Lock(0,0, &pVertices , 0);

		// fill it up
		memcpy(pVertices , VertexList , NumVertices * sizeof(MODEL_VERTEX));

	// Unlock VB
	pVB->Unlock();

	// Fill the Index Buffer
	unsigned short* pIndices;
	pIB->Lock(0,0,(void**)&pIndices,0);

		// Fill it up
		memcpy(pIndices , IndexList , NumIndices*sizeof(unsigned short));

	pIB->Unlock();

	return 1;

}

int Mesh3DS::RefreshVertexBuffer()
{

	// Fill the vb
	void* pVertices;
	// Lock the vb
	pVB->Lock(0,0, &pVertices , 0);

		// fill it up
		memcpy(pVertices , VertexList , NumVertices * sizeof(MODEL_VERTEX));

	// Unlock VB
	pVB->Unlock();

	return 1;

}

int Mesh3DS::RefreshIndexBuffer()
{

	// Fill the Index Buffer
	unsigned short* pIndices;
	pIB->Lock(0,0,(void**)&pIndices,0);

		// Fill it up
		memcpy(pIndices , IndexList , NumIndices*sizeof(unsigned short));

	pIB->Unlock();

	return 1;

}

int Mesh3DS::Render(PDIRECT3DDEVICE9 pDevice)
{

	// Begin the scene
	if (FAILED(pDevice->BeginScene()))
	{
		WriteToLog("Beginscene failed");
		return false;
	}
		// Set transform
		pDevice->SetTransform(D3DTS_WORLD , &Transform);

		// Set the stream source
		pDevice->SetStreamSource(0 , pVB , 0 , sizeof(MODEL_VERTEX));
		// Set the fvf
		pDevice->SetFVF(FVF_MODEL);
		// Set the index buffer
		pDevice->SetIndices( pIB );

		// Render the damn mesh!
		if (FAILED(pDevice->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST , 
			0 , 
			0,
			NumVertices,
			0,
			NumIndices/3)))
		{
			WriteToLog("<b>DrawPrimitive</b> failed!");
			return 0;
		}

	// End the scene
	pDevice->EndScene();
	
	return 1;

}

int Mesh3DS::BatchRender(PDIRECT3DDEVICE9 pDevice)
{


	// Set transform
	pDevice->SetTransform(D3DTS_WORLD , &Transform);

	// Set the stream source
	pDevice->SetStreamSource(0 , pVB , 0 , sizeof(MODEL_VERTEX));
	// Set the fvf
	pDevice->SetFVF(FVF_MODEL);
	// Set the index buffer
	pDevice->SetIndices( pIB );

	// Render the damn mesh!
	if (FAILED(pDevice->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST , 
		0 , 
		0,
		NumVertices,
		0,
		NumIndices/3)))
	{
		WriteToLog("<b>DrawPrimitive</b> failed!");
		return 0;
	}
	
	return 1;

}

void Mesh3DS::Release()
{

	// free the vertex and index buffers
	if (pVB) pVB->Release();
	if (pIB) pIB->Release();

	// Free the geometry memory
	if (VertexList) free(VertexList);
	if (IndexList) free(IndexList);
	if (pFaceNormals) free(pFaceNormals);

	// Free all neighbor Surfaces
	if (pNeighbors)
	{
	
		for (unsigned short index = 0;index<NumVertices;index++)
		{
		
			pNeighbors[index].Release();

		}

		free(pNeighbors);

	}

}

int Mesh3DS::CalculateFaceNormals(bool Norm)
{

	// See if there is memory allocated for the face normals
	if (!pFaceNormals)
	{
		WriteToLog("<b>CalculateFaceNormals </b>: No memory allocated for the normals!");
		return 0;
	}

	// calculate each triangle's normal
	D3DXVECTOR3 v1 , v2 , v3 , vertex1,vertex2,vertex3;
	for (unsigned short index=0;index<NumIndices/3;index++)
	{
		vertex1.x = VertexList[IndexList[3*index]].x;
		vertex1.y = VertexList[IndexList[3*index]].y;
		vertex1.z = VertexList[IndexList[3*index]].z;

		vertex2.x = VertexList[IndexList[3*index+1]].x;
		vertex2.y = VertexList[IndexList[3*index+1]].y;
		vertex2.z = VertexList[IndexList[3*index+1]].z;

		vertex3.x = VertexList[IndexList[3*index+2]].x;
		vertex3.y = VertexList[IndexList[3*index+2]].y;
		vertex3.z = VertexList[IndexList[3*index+2]].z;

		// find the 2 vectors we need
		v1 = vertex2 - vertex1;
		v2 = vertex3 - vertex1;

		// Calculate the normal
		D3DXVec3Cross(&v3,&v1,&v2);

		if (Norm) D3DXVec3Normalize(&v3,&v3);

		// Store the face normal
		pFaceNormals[index] = v3;
	}

	return 1;

}

int Mesh3DS::CalculateFaceNormalsFromVB()
{

	// See if there is memory allocated for the face normals
	if (!pFaceNormals)
	{
		WriteToLog("<b>CalculateFaceNormals </b>: No memory allocated for the normals!");
		return 0;
	}

	// Lock the vb
	MODEL_VERTEX* pVertices;
	pVB->Lock(0,0, (void**)&pVertices , 0);

	// calculate each triangle's normal
	D3DXVECTOR3 v1 , v2 , v3 , vertex1,vertex2,vertex3;
	for (unsigned short index=0;index<NumIndices/3;index++)
	{
		vertex1.x = pVertices[IndexList[3*index]].x;
		vertex1.y = pVertices[IndexList[3*index]].y;
		vertex1.z = pVertices[IndexList[3*index]].z;

		vertex2.x = pVertices[IndexList[3*index+1]].x;
		vertex2.y = pVertices[IndexList[3*index+1]].y;
		vertex2.z = pVertices[IndexList[3*index+1]].z;

		vertex3.x = pVertices[IndexList[3*index+2]].x;
		vertex3.y = pVertices[IndexList[3*index+2]].y;
		vertex3.z = pVertices[IndexList[3*index+2]].z;

		// find the 2 vectors we need
		v1 = vertex2 - vertex1;
		v2 = vertex3 - vertex1;

		// Calculate the normal
		D3DXVec3Cross(&v3,&v1,&v2);

		D3DXVec3Normalize(&v3,&v3);

		// Store the face normal
		pFaceNormals[index] = v3;
	}

	// Unlock VB
	pVB->Unlock();

	return 1;

}

int Mesh3DS::CalculateNormalsToVB()
{
	// See if there is memory allocated for the face normals
	if (!pFaceNormals)
	{
		WriteToLog("<b>CalculateNormals </b>: No memory allocated for the normals!");
		return 0;
	}

	// Lock the vb
	MODEL_VERTEX* pVertices;
	pVB->Lock(0,0,(void**)&pVertices , 0);

	// Now for each vertex determine the neighbouring surfaces
	// and do an average of the normals
	D3DXVECTOR3 average;
	for (unsigned short index=0;index<NumVertices;index++)
	{
		average = D3DXVECTOR3(0,0,0);

		for (unsigned short x=0;x<pNeighbors[index].Number;x++)
		{
			average += pFaceNormals[pNeighbors[index].Buffer[x]];
		}

		// divide the average
		if (pNeighbors[index].Number) average /=(float)pNeighbors[index].Number;

		// Normalize if asked to
		D3DXVec3Normalize(&average , &average);

		// Store the normal
		pVertices[index].nx = average.x;
		pVertices[index].ny = average.y;
		pVertices[index].nz = average.z;
	}

	// Unlock VB
	pVB->Unlock();

	return 1;
}

int Mesh3DS::FindNeighbors()
{

	// See if there is memory allocated for the neighbors
	if (!pNeighbors)
	{
		WriteToLog("<b>FindNeighbors </b>: No space allocated for the neighbor faces");
	
		return 0;
	}

	// First allocate some space
	unsigned short* temp;
	temp = (unsigned short *) malloc(1000*sizeof(unsigned short));

	// Now for each vertex determine the neighbouring surfaces
	unsigned short NumNeighborFaces;
	for (unsigned short vertex_index=0;vertex_index<NumVertices;vertex_index++)
	{
		NumNeighborFaces = 0;

		for (unsigned short edge_index=0;edge_index<NumIndices;edge_index++)
		{
			if (vertex_index == (IndexList[edge_index]))
			{
				// found a surface that contains this vertex
				temp[NumNeighborFaces] = edge_index/3;
				NumNeighborFaces ++;
			}
		}

		// Store the neighbors
		pNeighbors[vertex_index].Create(NumNeighborFaces);
		for (unsigned short x=0;x<NumNeighborFaces;x++)
		{
			pNeighbors[vertex_index].SetSurface(x,temp[x]);
		}

	}

	// free the temp space
	free(temp);

	return 1;

}

int Mesh3DS::CalculateNormals(bool Norm)
{

	// Now for each vertex determine the neighbouring surfaces
	// and do an average of the normals
	D3DXVECTOR3 average;
	for (unsigned short index=0;index<NumVertices;index++)
	{
		average = D3DXVECTOR3(0,0,0);

		for (unsigned short x=0;x<pNeighbors[index].Number;x++)
		{
			average += pFaceNormals[pNeighbors[index].Buffer[x]];
		}

		// divide the average
		if (pNeighbors[index].Number) average /=(float)pNeighbors[index].Number;

		// Normalize if asked to
		if (Norm) D3DXVec3Normalize(&average , &average);

		// Store the normal
		VertexList[index].nx = average.x;
		VertexList[index].ny = average.y;
		VertexList[index].nz = average.z;
	}

	return 1;

}

int Mesh3DS::AllocateSpace()
{

	// Create the face normal buffer
	// allocate space to store
	// every face normal
	pFaceNormals = (D3DXVECTOR3*)malloc(NumIndices/3*sizeof(D3DXVECTOR3));

	if (!pFaceNormals)
	{
		WriteToLog("<b>AllocateSpace </b>:Unable to allocate memory for the face normals");
		return 0;
	}

	// Allocate space for the neighbor surfaces
	pNeighbors = (NeighborSurfaces*) malloc(NumVertices * sizeof(NeighborSurfaces));
	if (!pNeighbors)
	{
		WriteToLog("<b>AllocateSpace </b>:Unable to allocate memory for Neighbouring faces");
		return 0;
	}

	return 1;
}

int Mesh3DS::CreateFrom(Mesh3DS* m)
{

	// Copy data
	NumVertices = m->NumVertices;
	NumIndices = m->NumIndices;

	// Allocate space for vertices
	VertexList = (MODEL_VERTEX*) malloc(NumVertices*sizeof(MODEL_VERTEX));

	if (!VertexList) 
	{
		WriteToLog("<b>CreateFrom </b>: Unable to Allocate memory for vertices");
		return 0;
	}

	// Allocate Space for faces
	IndexList = (unsigned short*) malloc(NumIndices*sizeof(unsigned short));

	if (!IndexList)
	{
		WriteToLog("<b>CreateFrom </b>: Unable to Allocate memory for Indices");
		return 0;
	}

	// Copy the indices
	memcpy(IndexList , m->IndexList , NumIndices*sizeof(unsigned short));


	return 1;

}

int Mesh3DS::LinearInterpolate(Mesh3DS* m1 , Mesh3DS* m2 , float t)
{
	// error check
	if (m1->NumVertices != m2->NumVertices)
	{
		WriteToLog("<b>LinearInterpolate </b>FAILED - mixed models do not have the same number of vertices");
		return 0;
	}
	if (m1->NumVertices != NumVertices) 
	{
		WriteToLog("<b>LinearInterpolate </b>FAILED - resulting model is not prepared correctly");
		return 0;
	}
	if (m1->NumIndices != m2->NumIndices) 
	{
		WriteToLog("<b>LinearInterpolate </b>FAILED - mixed models do not have the same number of indices");
		return 0;
	}
	if (m1->NumIndices != NumIndices) 
	{
		WriteToLog("<b>LinearInterpolate </b>FAILED - resulting model is not prepared correctly");
		return 0;
	}

	// the formula is:
	// C =  ( (1-t) * A + t * B ) 
	float one_minus_t;
	one_minus_t = 1.0f-t ;
	
	// point to the vertices list - optimisation
	MODEL_VERTEX *VertA , *VertB;
	VertA = m1->VertexList;
	VertB = m2->VertexList;

	// Interpolate all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		VertexList[vertex].x = one_minus_t * VertA[vertex].x +
							   t * VertB[vertex].x;
		VertexList[vertex].y = one_minus_t * VertA[vertex].y +
							   t * VertB[vertex].y;
		VertexList[vertex].z = one_minus_t * VertA[vertex].z +
							   t * VertB[vertex].z;

		VertexList[vertex].nx = one_minus_t * VertA[vertex].nx +
							   t * VertB[vertex].nx;
		VertexList[vertex].ny = one_minus_t * VertA[vertex].ny +
							   t * VertB[vertex].ny;
		VertexList[vertex].nz = one_minus_t * VertA[vertex].nz +
							   t * VertB[vertex].nz;
	}

	return 1;
}

int Mesh3DS::SphericalInterpolate(Mesh3DS* m1 , Mesh3DS* m2 , float t)
{
	// error check
	if (m1->NumVertices != m2->NumVertices)
	{
		WriteToLog("<b>SphericalInterpolate </b>FAILED");
		return 0;
	}
	if (m1->NumVertices != NumVertices) 
	{
		WriteToLog("<b>SphericalInterpolate </b>FAILED");
		return 0;
	}
	if (m1->NumIndices != m2->NumIndices) 
	{
		WriteToLog("<b>SphericalInterpolate </b>FAILED");
		return 0;
	}
	if (m1->NumIndices != NumIndices) 
	{
		WriteToLog("<b>SphericalInterpolate </b>FAILED");
		return 0;
	}

	// the formula is:
	// C =  ( (1-t) * A + t * B ) 
	float one_minus_t;
	one_minus_t = 1.0f-t ;

	float phi1 , theta1 , rho1,
		  phi2 , theta2 , rho2,
		  phi  , theta  , rho ,
		  l , tan_theta , cos_phi,
		  x,y,z;

	// Interpolate all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		// convert the vertices to spherical coordinates
		x = m1->VertexList[vertex].x;
		y = m1->VertexList[vertex].y;
		z = m1->VertexList[vertex].z;
		l = (float)sqrt(x*x + y*y + z*z);


		// if length == 0 we need not calculate the cos and tan
		if (l<0.0001f)
		{
			tan_theta = 0.0f;
			cos_phi = 0.0f;
		}
		else
		{
			if (x != 0) tan_theta = y / x;
			cos_phi = z / l;
		}

		phi1 = (float)acos(cos_phi);
		theta1 = (float)atan(tan_theta);
		if (x < 0) theta1 += D3DX_PI/2.0f;
		rho1 = l;

		// convert the vertices to spherical coordinates
		x = m2->VertexList[vertex].x;
		y = m2->VertexList[vertex].y;
		z = m2->VertexList[vertex].z;
		l = (float)sqrt(x*x + y*y + z*z);


		// if length == 0 we need not calculate the cos and tan
		if (l<0.0001f)
		{
			tan_theta = 0.0f;
			cos_phi = 0.0f;
		}
		else
		{
			if (x != 0) tan_theta = y / x;
			cos_phi = z / l;
		}

		phi2 = (float)acos(cos_phi);
		theta2 = (float)atan(tan_theta);
		if (x < 0) theta2 += D3DX_PI;
		rho2 = l;

		// Interpolate the spherical coordinates
		phi = phi1 * one_minus_t + phi2 * t;
		theta = theta1 * one_minus_t + theta2 * t;
		rho = rho1 * one_minus_t + rho2 * t;

		// Convert again to cartesian coordinates
		x = (float)(rho * sin(phi) * cos(theta));
		y = (float)(rho * sin(phi) * sin(theta));
		z = (float)(rho * cos(phi));

		VertexList[vertex].x = x;
		VertexList[vertex].y = y;
		VertexList[vertex].z = z;

		VertexList[vertex].nx = one_minus_t * m1->VertexList[vertex].nx +
							   t * m2->VertexList[vertex].nx;
		VertexList[vertex].ny = one_minus_t * m1->VertexList[vertex].ny +
							   t * m2->VertexList[vertex].ny;
		VertexList[vertex].nz = one_minus_t * m1->VertexList[vertex].nz +
							   t * m2->VertexList[vertex].nz;
	}
	

	return 1;

}

bool Mesh3DS::CreateCylinder(float Length , float Radius,int Slices,int CircleSlices)
{

	CylinderSlices = Slices;
	CylinderCircleSlices = CircleSlices;

	// Allocate space for vertices
	NumVertices = Slices*CircleSlices;
	VertexList = (MODEL_VERTEX*) malloc(NumVertices*sizeof(MODEL_VERTEX));

	// the cylinder is centered to the z-axis
	// so first find a slice and then move it
	for (int loop=0;loop<CircleSlices;loop++)
	{
		VertexList[loop].x = Radius*(float) cos((2*D3DX_PI*loop)/CircleSlices);
		VertexList[loop].y = Radius * (float) sin((2*D3DX_PI*loop)/CircleSlices);

		// Shift this slice to the negative z , for half the cylinder's
		// length
		VertexList[loop].z = 0;//-Length/2;

		VertexList[loop].tu = (float) loop / (CircleSlices-1);
		VertexList[loop].tv = 0;
	}

	// Now that we have the first slice we need to move it along z
	for (loop=1;loop<Slices;loop++)
	{
		for (int loop2=0;loop2<CircleSlices;loop2++)
		{
			VertexList[loop*CircleSlices+loop2].x = VertexList[(loop-1)*CircleSlices+loop2].x;
			VertexList[loop*CircleSlices+loop2].y = VertexList[(loop-1)*CircleSlices+loop2].y;
			VertexList[loop*CircleSlices+loop2].z = VertexList[(loop-1)*CircleSlices+loop2].z+Length/Slices;
			VertexList[loop*CircleSlices+loop2].tu = VertexList[(loop-1)*CircleSlices+loop2].tu;
			VertexList[loop*CircleSlices+loop2].tv = VertexList[(loop-1)*CircleSlices+loop2].tv + 1.0f/CircleSlices;
		}
		
	}

	// find normals
	for (loop=0;loop<NumVertices;loop++)
	{
		float d = VertexList[loop].x*VertexList[loop].x+VertexList[loop].y*VertexList[loop].y;

		d = (float) sqrt(d);

		VertexList[loop].nx = VertexList[loop].x/d;
		VertexList[loop].ny = VertexList[loop].y/d;
		VertexList[loop].nz = 0;
	}

	// Now find the indices
	// The number of sides per slice is the same as
	// CircleSlices . Every side produces 2 triangles
	// except from the last slice (end of cylinder)
	// so number of triangles = 2*CircleSlices*(Slices-1)
	NumIndices = 3*2*CircleSlices*(Slices-1);
	IndexList = (unsigned short*) malloc(NumIndices*sizeof(short));

	int this_side,next_side,this_side2,next_side2;
	int IndicesAdded = 0;

	// Add the triangles 2 by 2
	for (loop=0;loop<Slices-1;loop++)
	{
		for (int side=0;side<CircleSlices;side++)
		{
			// look where we are. If we are in the last side ,
			// the next vertex is the first of the slice
			// (to close the circle)
			if (side<CircleSlices-1)
			{
				this_side = loop*CircleSlices+side;
				next_side = this_side+1;

				this_side2 = this_side+CircleSlices;
				next_side2 = next_side + CircleSlices;

				// add two triangles
				IndexList[IndicesAdded] = this_side;
				IndicesAdded++;
				IndexList[IndicesAdded] = next_side2;
				IndicesAdded++;
				IndexList[IndicesAdded] = this_side2;
				IndicesAdded++;

				IndexList[IndicesAdded] = next_side;
				IndicesAdded++;
				IndexList[IndicesAdded] = next_side2;
				IndicesAdded++;
				IndexList[IndicesAdded] = this_side;
				IndicesAdded++;

			}
			else
			{
				this_side = loop*CircleSlices+side;
				next_side = this_side+1-CircleSlices;

				this_side2 = this_side+CircleSlices;
				next_side2 = next_side + CircleSlices;

				// add two triangles
				IndexList[IndicesAdded] = this_side;
				IndicesAdded++;
				IndexList[IndicesAdded] = next_side2;
				IndicesAdded++;
				IndexList[IndicesAdded] = this_side2;
				IndicesAdded++;

				IndexList[IndicesAdded] = next_side;
				IndicesAdded++;
				IndexList[IndicesAdded] = next_side2;
				IndicesAdded++;
				IndexList[IndicesAdded] = this_side;
				IndicesAdded++;
			}
		}
	}

	return true;

}

float psin(float angle)
{
	return (float) (sin(angle) + 1.0f) / 2.0f;
}

float pcos(float angle)
{
	return (float) (cos((double) angle) + 1.0f) / 2.0f;
}

void Mesh3DS::Distortion(unsigned long t)
{
	float time = t / 1000.0f;

	// Fill the vb
	MODEL_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0,(void**) &pVertices , 0);
		
		memcpy(pVertices , VertexList , sizeof(MODEL_VERTEX) * NumVertices);

		for (unsigned long vertex=0;vertex<NumVertices;vertex++)
		{

			D3DXVECTOR3 position , direction , unit , final;
			D3DXMATRIXA16  total , rotation , scaling;

			unit.x = 0; unit.y = 1 , unit.z = 0;

			position.x = VertexList[vertex].x;
			position.y = VertexList[vertex].y;
			position.z = VertexList[vertex].z;

			float z = position.z / 10;

			float sfact = 0.3f + ((psin(sin(time) + (float)cos(time/2)*z/8)+0.5f * pcos(time*2 + sin(z/5.0f)))/2);

			D3DXVec3Normalize(&direction , &position);
			float dotprd = D3DXVec3Dot(&direction , &unit);

			sfact *= psin( acos(dotprd)*(2.5f+psin(time)*2.0f)/*4.0f*/ ) + 1.0f;

			float rfact = (sin(time/3) + cos(z*sin(time/2)/6) + cos(time) + cos(z*sin(time/4)/3) + cos(time/2)) * 0.4f;
		
			D3DXMatrixRotationYawPitchRoll(&rotation,0,0,rfact);
			D3DXMatrixScaling(&scaling,sfact,sfact,1);
			D3DXMatrixMultiply(&total , &scaling , &rotation);

			// transform the vector
			D3DXVec3TransformCoord(&final,&position,&total);

			// find the normal
			float d = (float) sqrt(final.x*final.x + final.y*final.y);

			// write the vector to the vertex buffer
			pVertices[vertex].x = final.x;
			pVertices[vertex].y = final.y;
			pVertices[vertex].z = final.z;
	
			pVertices[vertex].nx = final.x / d;
			pVertices[vertex].ny = final.y / d;
			pVertices[vertex].nz = 0;//direction.z;
		}

	// Unlock VB
	pVB->Unlock();

}

// **********************************
// MeshGrid
// **********************************

int MeshGrid::CreateBuffers(PDIRECT3DDEVICE9 pDevice)
{
	// Create a vertex buffer
	pDevice->CreateVertexBuffer(
		NumVertices*sizeof(GRID_VERTEX),
		D3DUSAGE_WRITEONLY,
		FVF_GRID,
		D3DPOOL_SYSTEMMEM,
		&pVB,
		NULL);

	// Create the index buffer
	pDevice->CreateIndexBuffer(
		NumIndices * sizeof(unsigned short),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_SYSTEMMEM,
		&pIB,
		NULL);


	return 1;

}

int MeshGrid::RefreshBuffers()
{

	// Fill the vb
	void* pVertices;
	// Lock the vb
	pVB->Lock(0,0, &pVertices , 0);

		// fill it up
		memcpy(pVertices , VertexList , NumVertices*sizeof(GRID_VERTEX));

	// Unlock VB
	pVB->Unlock();

	// Fill the Index Buffer
	unsigned short* pIndices;
	pIB->Lock(0,0,(void**)&pIndices,0);

		// Fill it up
		memcpy(pIndices , IndexList , NumIndices*sizeof(unsigned short));

	pIB->Unlock();

	return 1;

}

void MeshGrid::ClearTransform()
{
	D3DXMatrixIdentity(&Transform);
}

void MeshGrid::SetTransform(TRANSFORM_TYPE t,
						   float par1,float par2,float par3)
{
	D3DXMATRIXA16 mat;
	switch (t)
	{
		case TRANSFORM_TRANSLATE:
			D3DXMatrixTranslation(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
		case TRANSFORM_ROTATE:
			D3DXMatrixRotationYawPitchRoll(&mat , par2,par1,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;

		case TRANSFORM_SCALE:
			D3DXMatrixScaling(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
	}
}

int MeshGrid::Render(PDIRECT3DDEVICE9 pDevice)
{

	// set the world matrix
	pDevice->SetTransform(D3DTS_WORLD , &Transform);

	// Begin the scene
	pDevice->BeginScene();

		// Set the stream source
		pDevice->SetStreamSource(0 , pVB , 0 , sizeof(GRID_VERTEX));
		// Set the fvf
		pDevice->SetFVF(FVF_GRID);
		// Set the index buffer
		pDevice->SetIndices( pIB );

		// Render the damn mesh!
		if (FAILED(pDevice->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST , 
			0 , 
			0,
			NumVertices,
			0,
			NumIndices/3)))
		{
			WriteToLog("<b>DrawPrimitive</b> failed!");
			return 0;
		}

	// End the scene
	pDevice->EndScene();
	
	return 1;

}

void MeshGrid::Release()
{

	// free the vertex and index buffers
	if (pVB) pVB->Release();
	if (pIB) pIB->Release();

	// Free the geometry memory
	if (VertexList) free(VertexList);
	if (IndexList) free(IndexList);

	if (Distances) free(Distances);
}

int MeshGrid::ApplyColor(unsigned long c)
{

	// Loop through all vertices
	for (unsigned short index = 0;index<NumVertices;index++)
	{
		VertexList[index].diffuse = c;
	}

	return 1;

}

int MeshGrid::CreateRectangle(unsigned short m , unsigned short n , 
		float width , float height)
{

	// First calculate vertices and indices
	NumVertices = m*n;
	NumIndices = 6 * (m-1)*(n-1);

	// find the x and y step
	float x_step , y_step;
	x_step = width / (float) (m-1);
	y_step = height / (float) (n-1);

	// Allocate space
	VertexList =  (GRID_VERTEX*) malloc(NumVertices * sizeof(GRID_VERTEX));
	IndexList = (unsigned short*) malloc(NumIndices * sizeof(unsigned short));

	Distances = (float *) malloc(NumVertices*sizeof(float));

	// loop through all rows
	for (unsigned short row=0;row<n;row++)
	{
		// loop through all columns
		for (unsigned short column=0;column<m;column++)
		{
			// Find the corresponding vertex number
			unsigned short vertex = column + m*row;

			// Assign coordinates and texture coordinates
			VertexList[vertex].x = -(width/2.0f) + column * x_step;
			VertexList[vertex].y = (height/2.0f) - row * y_step;
			VertexList[vertex].z = 0.0f;

			float u,v;
			u = (float) column / (float) (m-1);
			v = (float) row / (float) (n-1);

			VertexList[vertex].tu = u;
			VertexList[vertex].tv = v;

			// calculate the distances
			u-=0.5f;
			v-=0.5f;
			Distances[vertex] = (float)(2.0f*sqrt(u*u+v*v));

		}
	}

	// loop through all quads
	unsigned short index = 0;
	for (unsigned short quad_y=0;quad_y<(n-1);quad_y++)
	{

		for (unsigned short quad_x=0;quad_x<(m-1);quad_x++)

		{

			// Find the four indices
			//
			//     1          2
			//      __________
			//     |        / |
			//     |      /   |
			//     |    /     |
			//     |  /       |
			//     |/         |
			//      ----------
			//     4          3
			unsigned short i1,i2,i3,i4;
			i1 = quad_x + quad_y * m;
			i2 = i1 + 1;
			i3 = i2 + m;
			i4 = i3 - 1;

			// Assign the indices
			IndexList[index] = i1;
			index++;
			IndexList[index] = i2;
			index++;
			IndexList[index] = i4;
			index++;
			IndexList[index] = i2;
			index++;
			IndexList[index] = i3;
			index++;
			IndexList[index] = i4;
			index++;

		}
	}

	return 1;

}

int MeshGrid::VortexRectangle(float a)
{

	float u,v,sin_a,cos_a,d,angle;

	// Fill the vb
	GRID_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0,(void**) &pVertices , 0);


	// loop through all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		// Get the u,v of the vertex and convert them
		// to a plane (-1.0 , 1.0)
		u = VertexList[vertex].tu;
		v = VertexList[vertex].tv;

		u-=0.5f;
		u*=2.0f;
		v-=0.5f;
		v*=2.0f;

		// Find the distance
		d = Distances[vertex];

		if (d>0.0f)
		{
			// Find the angle to rotate
			angle = a / d;

			// find the sin and cos of the angle
			sin_a = (float)sin(angle);
			cos_a = (float)cos(angle);

			// rotate the point
			u = u*cos_a - v*sin_a;
			v = u*sin_a + v*cos_a;

			// Transform u,v again
			u/=2.0f;
			u+=0.5f;
			v/=2.0f;
			v+=0.5f;

			// Store them to the vertex buffer
			memcpy(pVertices+vertex , VertexList+vertex , sizeof(GRID_VERTEX));
			pVertices[vertex].tu = u;
			pVertices[vertex].tv = v;
		}
	}

	// Unlock VB
	pVB->Unlock();

	return 1;

}

int MeshGrid::Distort(float a)
{

	// Fill the vb
	GRID_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0,(void**) &pVertices , 0);

	float u,v;

	// loop through all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		// Get the u,v of the vertex and convert them
		// to a plane (-1.0 , 1.0)
		u = VertexList[vertex].tu;
		v = VertexList[vertex].tv;
/*
		u-=0.5f;
		u*=2.0f;
		v-=0.5f;
		v*=2.0f;
*/
		// Distort u and v
		u += 0.3f * sin(a+u+10*cos(u));
		v += 0.3f * cos(a+u+10*cos(u));
/*
		u /= 2.0f;
		u += 0.5f;
		v /= 2.0f;
		v += 0.5f;
*/
		// Store them to the vertex buffer
		memcpy(pVertices+vertex , VertexList+vertex , sizeof(GRID_VERTEX));
		pVertices[vertex].tu = u;
		pVertices[vertex].tv = v;
	}


	// Unlock VB
	pVB->Unlock();

	return 1;

}

int MeshGrid::Graviball(float x , float y , float a)
{

	float u,v,dx,dy,d,power;

	// Fill the vb
	GRID_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0,(void**) &pVertices , 0);


	// loop through all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		// Get the u,v of the vertex 
		u = VertexList[vertex].tu;
		v = VertexList[vertex].tv;

		// Find the distance
		dx = (x-u)/a;
		dy = (y-v)/a;
		d = (float) sqrt(dx*dx+dy*dy);
		//d = (dx+dy)/2.0f;




		if (d!=0.0f) 
		{
			power = a/d;
		}
		else 
		{
			power = 0.0f;
		}


		dx *= power;
		dy *= power;

		u += dx;
		v += dy;

		// Store them to the vertex buffer
		memcpy(pVertices+vertex , VertexList+vertex , sizeof(GRID_VERTEX));
		pVertices[vertex].tu = u;
		pVertices[vertex].tv = v;
		
	}

	// Unlock VB
	pVB->Unlock();

	return 1;

}


int	MeshGrid::RadialFill(float r)
{

	float d, best_radius , p;
	DWORD color;

	// Get the best radius
	best_radius = VertexList[0].y;

	// For all vertices
	for (unsigned short vertex=0;vertex<NumVertices;vertex++)
	{
		// Calculate the distance from the center
		d  = VertexList[vertex].x * VertexList[vertex].x;
		d += VertexList[vertex].y * VertexList[vertex].y;

		d = (float)sqrt(d);

		// if the outer radius is smaller than the inner 
		// we have nothing to do
		if (r>=best_radius)
		{
			ApplyColor(0xffffffff);
			return 1;
		}

		// else proceed
		d -= r;

		if (d>best_radius) d = best_radius;
		
		// if the vertex is inside the inner radius
		// do nothing
		if (d >= r)
		{
			p = d-r;
			p /= (best_radius-r);
			p=1.0f-p;
			color = (DWORD) (p*255);
			color <<= 24;
			VertexList[vertex].diffuse = color;

		}
		else
		{
			VertexList[vertex].diffuse = 0xffffffff;
		}

	}

	return 1;

}

void MeshGrid::Scroll(float x, float y)
{

	// Fill the vb
	GRID_VERTEX* pVertices;
	// Lock the vb
	pVB->Lock(0,0, (void**)&pVertices , 0);

		for (int index=0;index<NumVertices;index++)
		{
			pVertices[index].tu = VertexList[index].tu + x;
			pVertices[index].tv = VertexList[index].tv + y;
		}
	// Unlock VB
	pVB->Unlock();

}

// **********************************
// Material
// **********************************

int Material::LoadFrom3DS(char* filename , char* material_name)
{

	// Load a material
	float r,g,b;
	FILE *input = fopen(filename,"rb");

	// See if the file is valid
	if (!input)
	{
		return 0;
	}

	// Check if this is a valid 3ds 
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , input);

	if (chunk_id != CHUNK_MAIN) 
	{
		fclose(input);
		return 0;
	}

	std::uint32_t chunk_length;
	fread(&chunk_length, sizeof(chunk_length) , 1, input);

	// Go to the 3d editor chunk
	if (!GoToChunk(CHUNK_3DEDITOR , input))
	{
		fclose(input);
		return 0;
	}

	// Enumerate all materials of the file
	while (1)
	{

		chunk_length = GoToChunk(CHUNK_MATERIALBLOCK , input);

		if (!chunk_length) break;

		// Go to the material name chunk
		if (!GoToChunk(CHUNK_MATERIALNAME , input)) break;

		chunk_length -= 6;

		// Read the material's name
		char name;
		int name_index = 0;
		while ((name = getc(input)))
		{
			text_buffer1[name_index] = name;
			name_index++;
			chunk_length--;
		}
		text_buffer1[name_index] = 0;
		chunk_length--;



		if (!strcmp(text_buffer1 , material_name))
		{
			// This is our material. Load it!
			// Go to the ambient color chunk
			if (!GoToChunk(CHUNK_MATERIALAMBIENT,input))
			{
				//printf("No ambient material found\n");
				return 0;
			}
			if (!ReadColorChunk(&r,&g,&b,input))
			{
				//printf("Failed to load the color chunk\n");
				return 0;
			}

			//printf("Ambient:\n");
			//printf("R:%f\tG:%f\tB:%f\n",r,g,b);

			// Store the data read
			Data.Ambient.r = r;
			Data.Ambient.g = g;
			Data.Ambient.b = b;
			Data.Ambient.a = 1.0f;

			if (!GoToChunk(CHUNK_MATERIALDIFFUSE,input))
			{
				printf("No diffuse material found\n");
				return 0;
			}
			if (!ReadColorChunk(&r,&g,&b,input))
			{
				printf("Failed to load the color chunk\n");
				return 0;
			}

			// Store the data read
			Data.Diffuse.r = r;
			Data.Diffuse.g = g;
			Data.Diffuse.b = b;
			Data.Diffuse.a = 1.0f;

			if (!GoToChunk(CHUNK_MATERIALSPECULAR,input))
			{
				//printf("No diffuse material found\n");
				return 0;
			}
			if (!ReadColorChunk(&r,&g,&b,input))
			{
				//printf("Failed to load the color chunk\n");
				return 0;
			}

			// Store the data read
			Data.Specular.r = r;
			Data.Specular.g = g;
			Data.Specular.b = b;
			Data.Specular.a = 1.0f;

			// Read the percent power of the material
			if (!GoToChunk(CHUNK_MATERIALPOWER,input))
			{
				//printf("No power material found\n");
				return 0;
			}
			// Read the power percent
			float percent = 0;
			if (!ReadPercentChunk(&percent , input))
			{
				//printf("Couldnt load percent\n");
			}

			printf("Power: %f\n",percent);

			// Store the data read
			Data.Power = percent;

			// Find a texture map , if any
			if (GoToChunk(CHUNK_TEXTUREBLOCK,input))
			{
				GoToChunk(CHUNK_TEXTUREFILENAME , input);

				// Read the texture name
				int tname = 0;
				while (char c = getc(input))
				{
					Texture[tname] = c;
					tname++;
				}
				Texture[tname] = 0;

				HasTexture = true;
			}
			else
			{
				HasTexture = false;
			}

			fclose(input);

			// Sorry , no emmisive component in 3ds format
			Data.Emissive.r=0;
			Data.Emissive.g=0;
			Data.Emissive.b=0;
			Data.Emissive.a=0;

			//copy the material's name
			strcpy(Name , material_name);

			// Return success
			return 1;

		}
		else
		{
			// Skip this chunk
			//printf("Skipping object: %s" , text_buffer1);
			if (!SkipBytes(chunk_length-6,input)) 
			{
				fclose(input);
				return 0;
			}
		}


	} // End while


	// Close the file
	fclose(input);

	// Return
	return 0;

}

bool Material::IsTheName(char* n)
{

	int x=0;
	while (Name[x]!=0 && n[x]!=0)
	{
		if (Name[x]!=n[x]) return false;
		x++;
	}

	if (Name[x]) return false;
	if (n[x]) return false;

	return true;
}

void Material::Set(LPDIRECT3DDEVICE9 pDevice,IDirect3DBaseTexture9* Tex)
{
	pDevice->SetMaterial(&Data);

	if (HasTexture)
	{
		if (Tex)
			pDevice->SetTexture(0,Tex);
		else
			TexMan.SetTexture(pDevice , 0 , Texture);
	}
	else
	{
		if (Tex)
			pDevice->SetTexture(0,Tex);
		else
			pDevice->SetTexture(0,0);
	}
}

// **********************************
// MeshIso
// **********************************

int MeshIso::CreateBuffers(PDIRECT3DDEVICE9 pDevice)
{

	// Create a vertex buffer
	if (FAILED(pDevice->CreateVertexBuffer(
		NumVertices*sizeof(MODEL_VERTEX),
		D3DUSAGE_WRITEONLY,
		FVF_MODEL,
		D3DPOOL_SYSTEMMEM,
		&pVB,
		NULL)))
	{
		WriteToLog("<b>MeshIso::CreateBuffers()</b> - FAILED");
		return 0;
	}

	return 1;

}

int MeshIso::RefreshBuffers()
{

	// Fill the vb
	void* pVertices;
	// Lock the vb
	pVB->Lock(0,0, &pVertices , 0);

		// fill it up
		memcpy(pVertices , VertexList , NumTriangles * 3 * sizeof(MODEL_VERTEX));

	// Unlock VB
	pVB->Unlock();

	return 1;

}

int MeshIso::Render(PDIRECT3DDEVICE9 pDevice)
{

	// Begin the scene
	if (FAILED(pDevice->BeginScene()))
	{
		WriteToLog("Beginscene failed");
		return false;
	}

		// Set the stream source
		pDevice->SetStreamSource(0 , pVB , 0 , sizeof(MODEL_VERTEX));
		// Set the fvf
		pDevice->SetFVF(FVF_MODEL);

		// Render the damn mesh!
		if (FAILED(pDevice->DrawPrimitive(
			D3DPT_TRIANGLELIST , 
			0 , 
			NumTriangles)))
		{
			WriteToLog("<b>DrawPrimitive</b> failed!");
			return 0;
		}

	// End the scene
	pDevice->EndScene();

	
	return 1;

}

void MeshIso::Release()
{

	// free the vertex and index buffers
	if (pVB) pVB->Release();

	// Free the geometry memory
	if (VertexList) free(VertexList);

	// Free the balls memory
	if (pBalls) free(pBalls);

	// free the list memory
	if (ActiveCells) free(ActiveCells);

}

int	MeshIso::Create(unsigned short x_cells ,
				    unsigned short y_cells ,
				    unsigned short z_cells ,
				    float width,
				    float height,
				    float depth ,
					int num_balls)
{

	// Grid size
	NumCells = x_cells*y_cells*z_cells;

	// max number of triangles is 5*cells
	unsigned short MaxTris = NumCells*5;

	// Max vertices = Triangles*3
	//NumVertices = MaxTris*3;
	NumVertices = 10000;

	// Allocate space for Vertices
	VertexList = (MODEL_VERTEX*) malloc(sizeof(MODEL_VERTEX)*NumVertices);
	if (!VertexList)
	{
		WriteToLog("<b>MeshIso::Create()</b> - Couldnt allocate memor for the vertices");
		return 0;
	}
	memset(VertexList,0,sizeof(MODEL_VERTEX)*NumVertices);

	// Allocate space for the balls
	NumBalls = num_balls;
	pBalls = (MetaBall*) malloc(NumBalls*sizeof(MetaBall));

	GridWidth = width;
	GridHeight = height;
	GridDepth = depth;
	xCells = x_cells;
	yCells = y_cells;
	zCells = z_cells;

	// Allocate space for the bool values
	ActiveCells = (unsigned short*) malloc(10000*sizeof(unsigned short));

	if (!ActiveCells)
	{
		WriteToLog("<b>Couldnt allocate memory for the bool values - ActiveCells</b>");
		return 0;
	}

	return 1;

}

void MeshIso::Update()
{
	// Set tris to 0
	int tris_added = 0;
	NumTriangles = 0;
	NumActiveCells = 0;

	for (unsigned short ball=0;ball<NumBalls;ball++)
	{
		// Find the voxel that is near the ball's center
		float cell_w,cell_h,cell_d;
		cell_w = GridWidth /xCells;
		cell_h = GridHeight/yCells;
		cell_d = GridDepth /zCells;
		float half_w , half_h , half_d;
		half_w = GridWidth /2;
		half_h = GridHeight/2;
		half_d = GridDepth /2;
		
		// Start at -half_w and add until we have the x coord
		int x=0;
		while (cell_w*x-half_w<pBalls[ball].x) x++;
		x--;

		// Start at -half_h and add until we have the y coord
		int y=0;
		while (cell_h*y-half_h<pBalls[ball].y) y++;
		y--;

		// Start at -half_d and add until we have the z coord
		int z=0;
		while (cell_d*z-half_d<pBalls[ball].z) z++;
		z--;

		// choose where to move , left or right
		int move;
		if (x<xCells/2) move = -1;
		else move = 1;

		// loop until you find the surface
		
		while ( tris_added == 0 && x>=0 && x<xCells)
		{
			tris_added = CalculateCell(x,y,z);
			x += move;
		}

		// at last we found the surface. Now for the neighbors
		CalculateNeighbors(x,y,z);
	}


	// find the normal for each vertex
	// the normal is just the position normalized
	for (unsigned short vertex_index=0;vertex_index<NumTriangles*3;vertex_index++)
	{
		VertexList[vertex_index].nx = 0;
		VertexList[vertex_index].ny = 0;
		VertexList[vertex_index].nz = 0;

		for (unsigned short ball=0;ball<NumBalls;ball++)
		{
			float dx,dy,dz,d4;
			dx = VertexList[vertex_index].x - pBalls[ball].x;
			dy = VertexList[vertex_index].y - pBalls[ball].y;
			dz = VertexList[vertex_index].z - pBalls[ball].z;

			d4 = (dx*dx+dy*dy+dz*dz);
			d4 = d4*d4;


			VertexList[vertex_index].nx += 2*pBalls[ball].mass*dx/d4;
			VertexList[vertex_index].ny += 2*pBalls[ball].mass*dy/d4;
			VertexList[vertex_index].nz += 2*pBalls[ball].mass*dz/d4;
		}


		float l = (float)sqrt(VertexList[vertex_index].nx*VertexList[vertex_index].nx+
			VertexList[vertex_index].ny*VertexList[vertex_index].ny+
			VertexList[vertex_index].nz*VertexList[vertex_index].nz);
/*
		if (l < 0.0001f) l = 0.0001f;

		VertexList[vertex_index].nx /= l;
		VertexList[vertex_index].ny /= l;
		VertexList[vertex_index].nz /= l;
*/
	}

}

int	MeshIso::CalculateCell(int x , int y , int z)
{
	// Try to trivially remove the voxel
	if (x<0 || 
		x>xCells || 
		y<0 ||
		y>yCells ||
		z<0 || 
		z > zCells)
	{
		return 0;
	}


	// Calculate the corners of this voxel
	float cell_w,cell_h,cell_d;
	cell_w = GridWidth /xCells;
	cell_h = GridHeight/yCells;
	cell_d = GridDepth /zCells;
	float half_w , half_h , half_d;
	half_w = GridWidth /2;
	half_h = GridHeight/2;
	half_d = GridDepth /2;
	GRIDCELL curr_cell;

	//    4+----+5
	//   /     /|
 	// 0+----+1 |
	//  |    |  |
	//  | 7+ |  +6
	// 3+----+2/
	
	curr_cell.p[0].x = -half_w + x*cell_w;
	curr_cell.p[0].y = -half_h + (y-1)*cell_h;
	curr_cell.p[0].z = -half_d + z*cell_d;
	curr_cell.val[0] = 0;

	curr_cell.p[1].x = -half_w + (x+1)*cell_w;
	curr_cell.p[1].y = -half_h + (y-1)*cell_h;
	curr_cell.p[1].z = -half_d + z*cell_d;
	curr_cell.val[1] = 0;

	curr_cell.p[2].x = -half_w + (x+1)*cell_w;
	curr_cell.p[2].y = -half_h + y*cell_h;
	curr_cell.p[2].z = -half_d + z*cell_d;
	curr_cell.val[2] = 0;

	curr_cell.p[3].x = -half_w + x*cell_w;
	curr_cell.p[3].y = -half_h + y*cell_h;
	curr_cell.p[3].z = -half_d + z*cell_d;
	curr_cell.val[3] = 0;


	curr_cell.p[4].x = -half_w + x*cell_w;
	curr_cell.p[4].y = -half_h + (y-1)*cell_h;
	curr_cell.p[4].z = -half_d + (z+1)*cell_d;
	curr_cell.val[4] = 0;

	curr_cell.p[5].x = -half_w + (x+1)*cell_w;
	curr_cell.p[5].y = -half_h + (y-1)*cell_h;
	curr_cell.p[5].z = -half_d + (z+1)*cell_d;
	curr_cell.val[5] = 0;

	curr_cell.p[6].x = -half_w + (x+1)*cell_w;
	curr_cell.p[6].y = -half_h + y*cell_h;
	curr_cell.p[6].z = -half_d + (z+1)*cell_d;
	curr_cell.val[6] = 0;

	curr_cell.p[7].x = -half_w + x*cell_w;
	curr_cell.p[7].y = -half_h + y*cell_h;
	curr_cell.p[7].z = -half_d + (z+1)*cell_d;
	curr_cell.val[7] = 0;

	for (int v=0;v<8;v++)
	{

		for (unsigned short ball=0;ball<NumBalls;ball++)
		{
		float dx,dy,dz;
		dx = curr_cell.p[v].x - pBalls[ball].x;
		dy = curr_cell.p[v].y - pBalls[ball].y;
		dz = curr_cell.p[v].z - pBalls[ball].z;
		float d2 = (dx*dx+dy*dy+dz*dz);
		if (d2 < 0.0001f) d2 = 0.0001f;
		curr_cell.val[v] += pBalls[ball].mass/ d2;
		}
	}

	// Polygonize current cell
	int tris_added = Polygonise(curr_cell,
		IsoLevel , 
		(TRIANGLE*) (VertexList+NumTriangles*3));

	// Add it to done list
	AddCellToCalculated(x,y,z);

	NumTriangles += tris_added;
	return tris_added;
}


bool MeshIso::IsCellCalculated(int x , int y , int z)
{
	// Remove the voxels out of the grid as already calculated
	if (x<0 || 
		x>xCells || 
		y<0 ||
		y>yCells ||
		z<0 || 
		z > zCells)
	{
		return true;
	}

	unsigned short cell = x+y*xCells+z*xCells*yCells;

	for (unsigned short index = 0;index<NumActiveCells;index++)
	{
		if (ActiveCells[index] == cell) return true;
	}

	return false;

}


void MeshIso::AddCellToCalculated(int x , int y , int z)
{
	// Remove the voxels out of the grid as already calculated
	if (x<0 || 
		x>xCells || 
		y<0 ||
		y>yCells ||
		z<0 || 
		z > zCells)
	{
		return;
	}

	unsigned short cell = x+y*xCells+z*xCells*yCells;


	ActiveCells[NumActiveCells] = cell;
	NumActiveCells++;

}

inline bool MeshIso::CalculateNeighbors(int x,int y,int z)
{


	// Up
	if (!IsCellCalculated(x,y+1,z))
	{
		if (CalculateCell(x,y+1,z))
		{
			if (CalculateNeighbors(x,y+1,z))
			{
				return true;
			}
		}
	}

	// Down
	if (!IsCellCalculated(x,y-1,z))
	{
		if (CalculateCell(x,y-1,z))
		{
			if (CalculateNeighbors(x,y-1,z))
			{
				return true;
			}
		}
	}

	// Left
	if (!IsCellCalculated(x-1,y,z))
	{
		if (CalculateCell(x-1,y,z))
		{
			if (CalculateNeighbors(x-1,y,z))
			{
				return true;
			}
		}
	}

	// Right
	if (!IsCellCalculated(x+1,y,z))
	{
		if (CalculateCell(x+1,y,z))
		{
			if (CalculateNeighbors(x+1,y,z))
			{
				return true;
			}
		}
	}

	// Front
	if (!IsCellCalculated(x,y,z-1))
	{
		if (CalculateCell(x,y,z-1))
		{
			if (CalculateNeighbors(x,y,z-1))
			{
				return true;
			}
		}
	}

	// Front
	if (!IsCellCalculated(x,y,z+1))
	{
		if (CalculateCell(x,y,z+1))
		{
			if (CalculateNeighbors(x,y,z+1))
			{
				return true;
			}
		}
	}

	//***ADDED
	// Up-left
	if (!IsCellCalculated(x-1,y+1,z))
	{
		if (CalculateCell(x-1,y+1,z))
		{
			if (CalculateNeighbors(x-1,y+1,z))
			{
				return true;
			}
		}
	}

	// Down-left
	if (!IsCellCalculated(x-1,y-1,z))
	{
		if (CalculateCell(x-1,y-1,z))
		{
			if (CalculateNeighbors(x-1,y-1,z))
			{
				return true;
			}
		}
	}

	// Up-right
	if (!IsCellCalculated(x+1,y+1,z))
	{
		if (CalculateCell(x+1,y+1,z))
		{
			if (CalculateNeighbors(x+1,y+1,z))
			{
				return true;
			}
		}
	}

	// Down-right
	if (!IsCellCalculated(x+1,y-1,z))
	{
		if (CalculateCell(x+1,y-1,z))
		{
			if (CalculateNeighbors(x+1,y-1,z))
			{
				return true;
			}
		}
	}

	// Up-front
	if (!IsCellCalculated(x,y+1,z+1))
	{
		if (CalculateCell(x,y+1,z+1))
		{
			if (CalculateNeighbors(x,y+1,z+1))
			{
				return true;
			}
		}
	}

	// up-back
	if (!IsCellCalculated(x,y+1,z-1))
	{
		if (CalculateCell(x,y+1,z-1))
		{
			if (CalculateNeighbors(x,y+1,z-1))
			{
				return true;
			}
		}
	}

	// down-front
	if (!IsCellCalculated(x,y-1,z+1))
	{
		if (CalculateCell(x,y-1,z+1))
		{
			if (CalculateNeighbors(x,y-1,z+1))
			{
				return true;
			}
		}
	}

	// down-back
	if (!IsCellCalculated(x,y-1,z-1))
	{
		if (CalculateCell(x,y-1,z-1))
		{
			if (CalculateNeighbors(x,y-1,z-1))
			{
				return true;
			}
		}
	}


	return false;
}

// *** MeshInCube
int	MeshInCube::Create(float size)
{
	// 8 vertices , 36 indices
	NumVertices = 16;
	NumIndices = 36;

	// allocate memory
	VertexList = (GRID_VERTEX*) malloc(16*sizeof(GRID_VERTEX));
	IndexList = (unsigned short*) malloc(36*sizeof(unsigned short));
	

	// Fill the lists
	//    4+----+5
	//   /     /|
 	// 0+----+1 |
	//  |    |  |
	//  | 7+ |  +6
	// 3+----+2/

	VertexList[0].x			= -size/2;
	VertexList[0].y			=  size/2;
	VertexList[0].z			= -size/2;
	VertexList[0].tu		= 1.0f;
	VertexList[0].tv		= 0.0f;
	VertexList[0].diffuse	= 0xffffffff;

	VertexList[1].x			=  size/2;
	VertexList[1].y			=  size/2;
	VertexList[1].z			= -size/2;
	VertexList[1].tu		= 0.0f;
	VertexList[1].tv		= 0.0f;
	VertexList[1].diffuse	= 0xffffffff;

	VertexList[2].x			=  size/2;
	VertexList[2].y			= -size/2;
	VertexList[2].z			= -size/2;
	VertexList[2].tu		= 0.0f;
	VertexList[2].tv		= 1.0f;
	VertexList[2].diffuse	= 0xffffffff;

	VertexList[3].x			= -size/2;
	VertexList[3].y			= -size/2;
	VertexList[3].z			= -size/2;
	VertexList[3].tu		= 1.0f;
	VertexList[3].tv		= 1.0f;
	VertexList[3].diffuse	= 0xffffffff;

	VertexList[4].x			= -size/2;
	VertexList[4].y			=  size/2;
	VertexList[4].z			=  size/2;
	VertexList[4].tu		= 0.0f;
	VertexList[4].tv		= 0.0f;
	VertexList[4].diffuse	= 0xffffffff;

	VertexList[5].x			=  size/2;
	VertexList[5].y			=  size/2;
	VertexList[5].z			=  size/2;
	VertexList[5].tu		= 1.0f;
	VertexList[5].tv		= 0.0f;
	VertexList[5].diffuse	= 0xffffffff;;

	VertexList[6].x			=  size/2;
	VertexList[6].y			= -size/2;
	VertexList[6].z			=  size/2;
	VertexList[6].tu		= 1.0f;
	VertexList[6].tv		= 1.0f;
	VertexList[6].diffuse	= 0xffffffff;;

	VertexList[7].x			= -size/2;
	VertexList[7].y			= -size/2;
	VertexList[7].z			=  size/2;
	VertexList[7].tu		= 0.0f;
	VertexList[7].tv		= 1.0f;
	VertexList[7].diffuse	= 0xffffffff;


	//   12+----+13
	//   /     /|
 	// 8+----+9 |
	//  |    |  |
	//  |15+ |  +14
	//11+----+10
	// SIDE VERTICES
	VertexList[8].x			= -size/2;
	VertexList[8].y			=  size/2;
	VertexList[8].z			= -size/2;
	VertexList[8].tu		= 0.0f;
	VertexList[8].tv		= 0.0f;
	VertexList[8].diffuse	= 0xffffffff;

	VertexList[9].x			=  size/2;
	VertexList[9].y			=  size/2;
	VertexList[9].z			= -size/2;
	VertexList[9].tu		= 1.0f;
	VertexList[9].tv		= 0.0f;
	VertexList[9].diffuse	= 0xffffffff;

	VertexList[10].x			=  size/2;
	VertexList[10].y			= -size/2;
	VertexList[10].z			= -size/2;
	VertexList[10].tu		= 1.0f;
	VertexList[10].tv		= 1.0f;
	VertexList[10].diffuse	= 0xffffffff;

	VertexList[11].x			= -size/2;
	VertexList[11].y			= -size/2;
	VertexList[11].z			= -size/2;
	VertexList[11].tu		= 0.0f;
	VertexList[11].tv		= 1.0f;
	VertexList[11].diffuse	= 0xffffffff;

	VertexList[12].x			= -size/2;
	VertexList[12].y			=  size/2;
	VertexList[12].z			=  size/2;
	VertexList[12].tu		= 0.0f;
	VertexList[12].tv		= 1.0f;
	VertexList[12].diffuse	= 0xffffffff;

	VertexList[13].x			=  size/2;
	VertexList[13].y			=  size/2;
	VertexList[13].z			=  size/2;
	VertexList[13].tu		= 1.0f;
	VertexList[13].tv		= 1.0f;
	VertexList[13].diffuse	= 0xffffffff;;

	VertexList[14].x			=  size/2;
	VertexList[14].y			= -size/2;
	VertexList[14].z			=  size/2;
	VertexList[14].tu		= 1.0f;
	VertexList[14].tv		= 0.0f;
	VertexList[14].diffuse	= 0xffffffff;;

	VertexList[15].x			= -size/2;
	VertexList[15].y			= -size/2;
	VertexList[15].z			=  size/2;
	VertexList[15].tu		= 0.0f;
	VertexList[15].tv		= 0.0f;
	VertexList[15].diffuse	= 0xffffffff;

	// Indices
	IndexList[0]	= 1;
	IndexList[1]	= 0;
	IndexList[2]	= 3;

	IndexList[3]	= 1;
	IndexList[4]	= 3;
	IndexList[5]	= 2;

	IndexList[6]	= 4;
	IndexList[7]	= 5;
	IndexList[8]	= 7;

	IndexList[9]	= 5;
	IndexList[10]	= 6;
	IndexList[11]	= 7;

	IndexList[12]	= 8;
	IndexList[13]	= 13;
	IndexList[14]	= 12;

	IndexList[15]	= 8;
	IndexList[16]	= 9;
	IndexList[17]	= 13;

	IndexList[18]	= 15;
	IndexList[19]	= 14;
	IndexList[20]	= 11;

	IndexList[21]	= 14;
	IndexList[22]	= 10;
	IndexList[23]	= 11;

	IndexList[24]	= 0;
	IndexList[25]	= 4;
	IndexList[26]	= 3;

	IndexList[27]	= 4;
	IndexList[28]	= 7;
	IndexList[29]	= 3;

	IndexList[30]	= 5;
	IndexList[31]	= 1;
	IndexList[32]	= 2;

	IndexList[33]	= 6;
	IndexList[34]	= 5;
	IndexList[35]	= 2;

	/*

	// Indices
	IndexList[0]	= 1;
	IndexList[1]	= 0;
	IndexList[2]	= 3;

	IndexList[3]	= 1;
	IndexList[4]	= 3;
	IndexList[5]	= 2;

	IndexList[6]	= 4;
	IndexList[7]	= 5;
	IndexList[8]	= 7;

	IndexList[9]	= 5;
	IndexList[10]	= 6;
	IndexList[11]	= 7;

	IndexList[12]	= 0;
	IndexList[13]	= 5;
	IndexList[14]	= 4;

	IndexList[15]	= 0;
	IndexList[16]	= 1;
	IndexList[17]	= 5;

	IndexList[18]	= 7;
	IndexList[19]	= 6;
	IndexList[20]	= 3;

	IndexList[21]	= 6;
	IndexList[22]	= 2;
	IndexList[23]	= 3;

	IndexList[24]	= 0;
	IndexList[25]	= 4;
	IndexList[26]	= 3;

	IndexList[27]	= 4;
	IndexList[28]	= 7;
	IndexList[29]	= 3;

	IndexList[30]	= 5;
	IndexList[31]	= 1;
	IndexList[32]	= 2;

	IndexList[33]	= 6;
	IndexList[34]	= 5;
	IndexList[35]	= 2;
	*/

	return 1;
}

int MeshInCube::CreateBuffers(PDIRECT3DDEVICE9 pDevice)
{
	// Create a vertex buffer
	pDevice->CreateVertexBuffer(
		NumVertices*sizeof(GRID_VERTEX),
		D3DUSAGE_WRITEONLY,
		FVF_GRID,
		D3DPOOL_SYSTEMMEM,
		&pVB,
		NULL);

	// Create the index buffer
	pDevice->CreateIndexBuffer(
		NumIndices * sizeof(unsigned short),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_SYSTEMMEM,
		&pIB,
		NULL);


	return 1;

}

int MeshInCube::RefreshBuffers()
{

	// Fill the vb
	void* pVertices;
	// Lock the vb
	pVB->Lock(0,0, &pVertices , 0);

		// fill it up
		memcpy(pVertices , VertexList , NumVertices*sizeof(GRID_VERTEX));

	// Unlock VB
	pVB->Unlock();

	// Fill the Index Buffer
	unsigned short* pIndices;
	pIB->Lock(0,0,(void**)&pIndices,0);

		// Fill it up
		memcpy(pIndices , IndexList , NumIndices*sizeof(unsigned short));

	pIB->Unlock();

	return 1;

}

int MeshInCube::Render(PDIRECT3DDEVICE9 pDevice)
{
	// Begin the scene
	pDevice->BeginScene();

		// Set the stream source
		pDevice->SetStreamSource(0 , pVB , 0 , sizeof(GRID_VERTEX));
		// Set the fvf
		pDevice->SetFVF(FVF_GRID);
		// Set the index buffer
		pDevice->SetIndices( pIB );

		// Render the damn mesh!
		if (FAILED(pDevice->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST , 
			0 , 
			0,
			NumVertices,
			0,
			NumIndices/3)))
		{
			WriteToLog("<b>DrawPrimitive</b> failed!");
			return 0;
		}

	// End the scene
	pDevice->EndScene();
	
	return 1;

}


void MeshInCube::Release()
{

	// free the vertex and index buffers
	if (pVB) pVB->Release();
	if (pIB) pIB->Release();

	// Free the geometry memory
	if (VertexList) free(VertexList);
	if (IndexList) free(IndexList);

}

int MeshInCube::ApplyColor(unsigned long c)
{

	// Loop through all vertices
	for (unsigned short index = 0;index<NumVertices;index++)
	{
		VertexList[index].diffuse = c;
	}

	return 1;

}

// ***************************
//	MaterialManager
// ***************************

int MaterialManager::LoadFrom3DS(char* filename)
{

	// Enumerate all the materials in 
	// this file and then load them one by one

	// Allocate memory for the material names
	char* material_names = (char*) malloc(40*1000);

	// Open the 3ds file
	FILE *input = fopen(filename,"r+b");

	// See if the file is valid
	if (!input)
	{
		return 0;
	}

	// Check if this is a valid 3ds 
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , input);

	if (chunk_id != CHUNK_MAIN) 
	{
		fclose(input);
		return 0;
	}

	unsigned long chunk_length;
	fread(&chunk_length, sizeof(unsigned long) , 1, input);

	// Go to the 3d editor chunk
	if (!GoToChunk(CHUNK_3DEDITOR , input))
	{
		fclose(input);
		return 0;
	}

	int material_number=0;
	// Enumerate all materials of the file
	while (!feof(input))
	{

		chunk_length = GoToChunk(CHUNK_MATERIALBLOCK , input);

		if (!chunk_length) break;

		// Go to the material name chunk
		if (!GoToChunk(CHUNK_MATERIALNAME , input)) break;

		chunk_length -= 6;

		// Read the material's name
		char name;
		int name_index = 0;
		while ((name = getc(input)))
		{
			text_buffer1[name_index] = name;
			name_index++;
			chunk_length--;
		}
		text_buffer1[name_index] = 0;
		chunk_length--;

		// Copy the name into the list
		strcpy(material_names+40*material_number , text_buffer1);
		material_number++;

		// Skip this chunk
		SkipBytes(chunk_length-6,input);

	} // End while


	// Close the file
	fclose(input);


	// Now load all the materials one by one
	if (material_number == 0) return 0;

	NumMaterials = material_number;

	// Allocate memory for the materials
	MaterialList = (Material*) malloc(NumMaterials * sizeof(Material));

	for (unsigned long m_index=0;m_index<NumMaterials;m_index++)
	{
		MaterialList[m_index].LoadFrom3DS(filename , material_names+40*m_index);
	}

	// Deallocate name memory
	free(material_names);

	return 1;

}

void MaterialManager::Release()
{
	if (MaterialList)
		free(MaterialList);
}

Material* MaterialManager::GetMaterial(char* n)
{
	for (unsigned long x=0;x<NumMaterials;x++)
	{
		if (MaterialList[x].IsTheName(n))
			return MaterialList+x;
	}

	return 0;
}

void MaterialManager::SetMaterial(LPDIRECT3DDEVICE9 pDevice , char* n,
													IDirect3DBaseTexture9* Tex)
{

	for (unsigned long x=0;x<NumMaterials;x++)
	{
		if (MaterialList[x].IsTheName(n))
			MaterialList[x].Set(pDevice,Tex);
	}

}

// **************************
// Scene3DS
// **************************

Scene3DS::Scene3DS()
{
	NumObjects = 0;
	ClearTransform();
}

int Scene3DS::LoadFrom3DS(LPDIRECT3DDEVICE9 pDevice,
						  char* filename)
{

	// Enumerate all objects of a scene and 
	// then load them one by one

	// Allocate temp space for the object names
	char* object_names = (char*) malloc(40 * 1000);

	FILE *input = fopen(filename,"r+b");

	// See if the file is valid
	if (!input)
	{
		WriteToLog("No such file:");
		WriteToLog(filename);
		return 0;
	}

	// Check if this is a valid 3ds 
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , input);

	if (chunk_id != CHUNK_MAIN) 
	{
		WriteToLog("This is not a valid 3DS file:");
		WriteToLog(filename);
		fclose(input);
		return 0;
	}

	unsigned long chunk_length;
	fread(&chunk_length, sizeof(unsigned long) , 1, input);

	// Go to the 3d editor chunk
	if (!GoToChunk(CHUNK_3DEDITOR , input))
	{
		WriteToLog("This is not a valid 3DS file:");
		WriteToLog(filename);
		fclose(input);
		return 0;
	}

	unsigned long object_counter = 0;

	// Enumerate all objects of the file
	while (!feof(input))
	{

		chunk_length = GoToChunk(CHUNK_OBJECTBLOCK , input);

		if (!chunk_length)  break;

		// Read the objects name
		char name;
		int name_index = 0;
		while ((name = getc(input)))
		{
			text_buffer1[name_index] = name;
			name_index++;
			chunk_length--;
		}
		text_buffer1[name_index] = 0;
		chunk_length--;

		strcpy(object_names+40*object_counter , text_buffer1);
		object_counter++;

		// Skip this chunk
		if (!SkipBytes(chunk_length-6,input)) break;

	} // End while

	// Close the file
	fclose(input);


	// Now we are ready to load all the objects of the scene

	// Allocate memory
	ObjectList = (Mesh3DS*) malloc(object_counter*sizeof(Mesh3DS));
	NumObjects = object_counter;

	object_counter = 0;
	// load them
	for (unsigned long x=0;x<NumObjects;x++)
	{
		if (ObjectList[object_counter].LoadFrom3DS(filename , object_names+40*x))
		{
			ObjectList[object_counter].AllocateSpace();
			ObjectList[object_counter].CalculateFaceNormals(false);
			ObjectList[object_counter].FindNeighbors();
			ObjectList[object_counter].CalculateNormals(true);
			ObjectList[object_counter].CreateBuffers(pDevice);
			ObjectList[object_counter].RefreshBuffers();
			ObjectList[object_counter].ComputeBoundingBox();

			object_counter++;
		}

	}

	// NumObjects gets decreased a little . We load only
	// meshes here , so lights , lines , cameras etc...
	// will be a little garbage here. They will
	// anyway be wiped out with free when we release the
	// scene , plus they will not occupy memory
	NumObjects = object_counter;

	// deallocate temp space
	free(object_names);

	// Load the Material Manager
	MaMa.LoadFrom3DS(filename);

	// return success
	return 1;
}

void Scene3DS::Release()
{

	if (ObjectList)
	{
		for (unsigned long x=0;x<NumObjects;x++)
		{
			ObjectList[x].Release();
		}
		free(ObjectList);
	}
}

Mesh3DS* Scene3DS::GetObject(char* n)
{
	
	for (unsigned long x=0;x<NumObjects;x++)
	{
		if (ObjectList[x].IsTheName(n))
			return ObjectList+x;
	}

	return 0;
}

void Scene3DS::RenderObject(LPDIRECT3DDEVICE9 pDevice , char* n)
{

	for (unsigned long x=0;x<NumObjects;x++)
	{
		if (ObjectList[x].IsTheName(n))
		{
			D3DXMatrixMultiply(&ObjectList[x].Transform,
				&Transform,
				&ObjectList[x].Transform);
			ObjectList[x].Render(pDevice);
		}
	}

}


void Scene3DS::Render(LPDIRECT3DDEVICE9 pDevice)
{
	// Begin the scene
	pDevice->BeginScene();

	// loop for all objects
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// Set the object's material
		MaMa.SetMaterial(pDevice , ObjectList[x].MaterialName,
			ObjectList[x].ExternalTexture);

		D3DXMatrixMultiply(&ObjectList[x].Transform,
			&Transform,
			&ObjectList[x].Transform);

		// Render
		ObjectList[x].BatchRender(pDevice);
	}

	// End the Scene
	pDevice->EndScene();
}

unsigned long Scene3DS::RenderClipped(LPDIRECT3DDEVICE9 pDevice,
							 CULLINFO* frustum)
{
	unsigned long objects_clipped = 0;

	// Begin the scene
	pDevice->BeginScene();

	// loop for all objects
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// Set the object's material
		MaMa.SetMaterial(pDevice , ObjectList[x].MaterialName,
			ObjectList[x].ExternalTexture);

		D3DXMatrixMultiply(&ObjectList[x].Transform,
			&Transform,
			&ObjectList[x].Transform);

		// Render
		if (!ObjectList[x].BatchRenderClipped(pDevice,frustum))
		{
			objects_clipped++;
		}


	}

	// End the scene
	pDevice->EndScene();

	return objects_clipped;
}

void Scene3DS::BatchRender(LPDIRECT3DDEVICE9 pDevice)
{

	// loop for all objects
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// Set the object's material
		MaMa.SetMaterial(pDevice , ObjectList[x].MaterialName,
			ObjectList[x].ExternalTexture);

		D3DXMatrixMultiply(&ObjectList[x].Transform,
			&Transform,
			&ObjectList[x].Transform);

		// Render
		ObjectList[x].BatchRender(pDevice);
	}
}

unsigned long Scene3DS::BatchRenderClipped(LPDIRECT3DDEVICE9 pDevice,
							 CULLINFO* frustum)
{
	unsigned long objects_clipped = 0;

	// loop for all objects
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// Set the object's material
		MaMa.SetMaterial(pDevice , ObjectList[x].MaterialName,
			ObjectList[x].ExternalTexture);

		D3DXMatrixMultiply(&ObjectList[x].Transform,
			&Transform,
			&ObjectList[x].Transform);

		// Render
		if (!ObjectList[x].BatchRenderClipped(pDevice,frustum))
		{
			objects_clipped++;
		}


	}

	return objects_clipped;
}


void Scene3DS::SaveToS3D(char* filename)
{
	// open the output file
	FILE* output = fopen(filename , "w+b");

	// First write number of objects
	fwrite(&NumObjects , sizeof(NumObjects) , 1 , output);

	// Then loop for all objects and write...
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// The name
		fwrite(ObjectList[x].Name , 1 , 40 , output);

		// Number of vertices
		fwrite(&ObjectList[x].NumVertices , sizeof(ObjectList[x].NumVertices) , 1 , output);

		// Vertices
		fwrite(ObjectList[x].VertexList , sizeof(MODEL_VERTEX) , ObjectList[x].NumVertices , output);

		// Number of indices
		fwrite(&ObjectList[x].NumIndices , sizeof(ObjectList[x].NumIndices) , 1 , output);

		// Indices
		fwrite(ObjectList[x].IndexList , sizeof(unsigned short) , ObjectList[x].NumIndices , output);

		// Bounding Box
		fwrite(ObjectList[x].BoundingBox , sizeof(D3DXVECTOR3) , 8 , output);

		// Object's Material
		fwrite(ObjectList[x].MaterialName , 1 , 40 , output);
	} // End object loop

	// And now the materials...
	fwrite(&MaMa.NumMaterials , sizeof(MaMa.NumMaterials) , 1 , output);

	// Write all materials
	fwrite(MaMa.MaterialList , sizeof(Material) , MaMa.NumMaterials , output);


	// Done. close the file
	fclose(output);
}

int Scene3DS::LoadFromS3D(char* filename , LPDIRECT3DDEVICE9 pDevice)
{

	// Open the input file
	FILE* input = fopen(filename , "rb");

	if (!input) return 0;

	// First read number of objects
	fread(&NumObjects , sizeof(NumObjects) , 1 , input);

	// Allocate memory for each object
	ObjectList = new Mesh3DS[NumObjects];

	// Then loop for all objects and read...
	for (unsigned long x=0;x<NumObjects;x++)
	{
		// The name
		fread(ObjectList[x].Name , 1 , 40 , input);

		// Number of vertices
		fread(&ObjectList[x].NumVertices , sizeof(ObjectList[x].NumVertices) , 1 , input);

		// Allocate memory for the vertices
		ObjectList[x].VertexList = (MODEL_VERTEX*) malloc(ObjectList[x].NumVertices*sizeof(MODEL_VERTEX));

		// Vertices
		fread(ObjectList[x].VertexList , sizeof(MODEL_VERTEX) , ObjectList[x].NumVertices , input);

		// Number of indices
		fread(&ObjectList[x].NumIndices , sizeof(ObjectList[x].NumIndices) , 1 , input);

		// Allocate memory for the indices
		ObjectList[x].IndexList = (unsigned short*) malloc(sizeof(unsigned short)*ObjectList[x].NumIndices);

		// Indices
		fread(ObjectList[x].IndexList , sizeof(unsigned short) , ObjectList[x].NumIndices , input);

		// Bounding Box
		fread(ObjectList[x].BoundingBox , sizeof(D3DXVECTOR3) , 8 , input);

		ObjectList[x].SetBBIndices();

		// Object's Material
		fread(ObjectList[x].MaterialName , 1 , 40 , input);

		// By default the object has no external texture
		ObjectList[x].ExternalTexture = NULL;

		// Create vertex/index buffers and fill them up
		ObjectList[x].CreateBuffers(pDevice);
		ObjectList[x].RefreshBuffers();
	} // End object loop

	// And now the materials...
	fread(&MaMa.NumMaterials , sizeof(MaMa.NumMaterials) , 1 , input);

	// Allocate space for the materials
	MaMa.MaterialList = (Material*) malloc(sizeof(Material) * MaMa.NumMaterials);

	// Read all materials
	fread(MaMa.MaterialList , sizeof(Material) , MaMa.NumMaterials , input);

	// Done. close the file
	fclose(input);

	// Load all the textures of the scene
	for (unsigned long i=0;i<MaMa.NumMaterials; i++)
	{
		if (MaMa.MaterialList[i].HasTexture) 
			TexMan.LoadTexture(pDevice , MaMa.MaterialList[i].Texture);
	}

	return 1;
}

void Scene3DS::ClearTransform()
{
	D3DXMatrixIdentity(&Transform);

	for (unsigned long x=0;x<NumObjects;x++)
	{
		ObjectList[x].ClearTransform();
	}
}

void Scene3DS::SetTransform(TRANSFORM_TYPE t,
						   float par1,float par2,float par3)
{
	D3DXMATRIXA16 mat;
	switch (t)
	{
		case TRANSFORM_TRANSLATE:
			D3DXMatrixTranslation(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
		case TRANSFORM_ROTATE:
			D3DXMatrixRotationYawPitchRoll(&mat , par2,par1,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;

		case TRANSFORM_SCALE:
			D3DXMatrixScaling(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
	}
}




/////////////////////////////////////////////////////////////

Particle::Particle()
{
	x=0; y=0; z=0;
	color = 0;
	size = 0;
	alive = 0;
}

/////////////////////////////////////////////////////////////
MeshBillboard::MeshBillboard()
{
	ParticleList		=NULL;
	NumParticles		=0;

	pVB					=NULL;

	D3DXMatrixIdentity(&Transform);
}

int MeshBillboard::Init(PDIRECT3DDEVICE9 pDevice , unsigned long num_particles)
{
	// allocate space for each particle
	NumParticles = num_particles;
	ParticleList = new Particle[num_particles];

	// allocate space for vertices
	VertexList = (BILLBOARD_VERTEX*) malloc(600 * sizeof(BILLBOARD_VERTEX));

	// create buffers
	CreateBuffers(pDevice);

	return 1;
}

int	MeshBillboard::CreateBuffers(PDIRECT3DDEVICE9 pDevice)
{
	// we need to print 100 quads at a time , so 400 vertices...

	// Create a vertex buffer
	pDevice->CreateVertexBuffer(
		600*sizeof(BILLBOARD_VERTEX),
		D3DUSAGE_WRITEONLY,
		FVF_BILLBOARD,
		D3DPOOL_SYSTEMMEM,
		&pVB,
		NULL);

	return 1;
}

int	MeshBillboard::Render(PDIRECT3DDEVICE9 pDevice)
{
	unsigned long ParticlesToRender , ParticlesRendered;
	ParticlesToRender = 0;
	ParticlesRendered = 0;

	unsigned long particle_index = 0;

	bool	render_last = false;

	D3DXMATRIXA16	trans,particle_translation,final;	

	// get the view matrix
	pDevice->GetTransform(D3DTS_VIEW , &trans);
	trans._14 = 0;
	trans._24 = 0;
	trans._34 = 0;
	trans._44 = 1;
	D3DXMatrixInverse(&trans ,NULL, &trans);

	while (ParticlesRendered < NumActiveParticles() && render_last == false)
	{
		// add a particle

		//		find the next alive particle
		while (!ParticleList[particle_index].alive)
		{
			particle_index++;
			if (particle_index == NumParticles) return 1;
			if (particle_index >= NumParticles-1) render_last = true;
		}


		//		make a translation matrix with the particle's position
		D3DXMatrixTranslation(&particle_translation , 
			ParticleList[particle_index].x,
			ParticleList[particle_index].y,
			ParticleList[particle_index].z);

		//		make the last transformation matrix for this particle as particle_translation x mesh_transform
		D3DXMatrixMultiply(&particle_translation , &particle_translation , &Transform);
		
		//		plug the final particle's position in the inverse rotation of the view matrix
		final = trans;
		final._41 = particle_translation._41;
		final._42 = particle_translation._42;
		final._43 = particle_translation._43;

		//		make a quad around the point (0,0,0) with the particle's size
		D3DXVECTOR3	ul,ur,dl,dr;
		ul.x = -ParticleList[particle_index].size/2;
		ul.y =  ParticleList[particle_index].size/2;
		ul.z =  0;

		ur.x =  ParticleList[particle_index].size/2;
		ur.y =  ParticleList[particle_index].size/2;
		ur.z =  0;

		dl.x = -ParticleList[particle_index].size/2;
		dl.y = -ParticleList[particle_index].size/2;
		dl.z =  0;

		dr.x =  ParticleList[particle_index].size/2;
		dr.y = -ParticleList[particle_index].size/2;
		dr.z =  0;

		//		transform the quad with the above trans matrix
		D3DXVec3TransformCoord(&ul , &ul , &final);
		D3DXVec3TransformCoord(&ur , &ur , &final);
		D3DXVec3TransformCoord(&dl , &dl , &final);
		D3DXVec3TransformCoord(&dr , &dr , &final);

		//		store the quad as 6 vertices in VertexList[6*ParticlesToRender]
		VertexList[6*ParticlesToRender + 0].x			= ul.x;
		VertexList[6*ParticlesToRender + 0].y			= ul.y;
		VertexList[6*ParticlesToRender + 0].z			= ul.z;
		VertexList[6*ParticlesToRender + 0].tu			= 0.0f;
		VertexList[6*ParticlesToRender + 0].tv			= 0.0f;
		VertexList[6*ParticlesToRender + 0].diffuse		= ParticleList[particle_index].color;

		VertexList[6*ParticlesToRender + 1].x			= ur.x;
		VertexList[6*ParticlesToRender + 1].y			= ur.y;
		VertexList[6*ParticlesToRender + 1].z			= ur.z;
		VertexList[6*ParticlesToRender + 1].tu			= 1.0f;
		VertexList[6*ParticlesToRender + 1].tv			= 0.0f;
		VertexList[6*ParticlesToRender + 1].diffuse		= ParticleList[particle_index].color;

		VertexList[6*ParticlesToRender + 2].x			= dl.x;
		VertexList[6*ParticlesToRender + 2].y			= dl.y;
		VertexList[6*ParticlesToRender + 2].z			= dl.z;
		VertexList[6*ParticlesToRender + 2].tu			= 0.0f;
		VertexList[6*ParticlesToRender + 2].tv			= 1.0f;
		VertexList[6*ParticlesToRender + 2].diffuse		= ParticleList[particle_index].color;

		VertexList[6*ParticlesToRender + 3].x			= dl.x;
		VertexList[6*ParticlesToRender + 3].y			= dl.y;
		VertexList[6*ParticlesToRender + 3].z			= dl.z;
		VertexList[6*ParticlesToRender + 3].tu			= 0.0f;
		VertexList[6*ParticlesToRender + 3].tv			= 1.0f;
		VertexList[6*ParticlesToRender + 3].diffuse		= ParticleList[particle_index].color;

		VertexList[6*ParticlesToRender + 4].x			= ur.x;
		VertexList[6*ParticlesToRender + 4].y			= ur.y;
		VertexList[6*ParticlesToRender + 4].z			= ur.z;
		VertexList[6*ParticlesToRender + 4].tu			= 1.0f;
		VertexList[6*ParticlesToRender + 4].tv			= 0.0f;
		VertexList[6*ParticlesToRender + 4].diffuse		= ParticleList[particle_index].color;

		VertexList[6*ParticlesToRender + 5].x			= dr.x;
		VertexList[6*ParticlesToRender + 5].y			= dr.y;
		VertexList[6*ParticlesToRender + 5].z			= dr.z;
		VertexList[6*ParticlesToRender + 5].tu			= 1.0f;
		VertexList[6*ParticlesToRender + 5].tv			= 1.0f;
		VertexList[6*ParticlesToRender + 5].diffuse		= ParticleList[particle_index].color;

		// advance ParticlesToRender
		ParticlesToRender++;
		particle_index++;

		if (ParticlesToRender == 99 || ParticlesRendered == NumActiveParticles() - ParticlesToRender )
		{
			//	render (particlesToRender) particles
				// fill the vb
			void* pVertices;
			// Lock the vb
			pVB->Lock(0,0, &pVertices , 0);

				// fill it up
				memcpy(pVertices , VertexList , ParticlesToRender * 6 * sizeof(BILLBOARD_VERTEX));

			// Unlock VB
			pVB->Unlock();
/*
			// save the world transform
			D3DXMATRIXA16 w , iden;
			pDevice->GetTransform(D3DTS_WORLD , &w);
			D3DXMatrixIdentity(&iden);
			pDevice->SetTransform(D3DTS_WORLD , &iden);
*/
				// render
			// Begin the scene
			if (FAILED(pDevice->BeginScene()))
			{
				WriteToLog("Beginscene failed");
			}
				
				// Set the stream source
				pDevice->SetStreamSource(0 , pVB , 0 , sizeof(BILLBOARD_VERTEX));
				// Set the fvf
				pDevice->SetFVF(FVF_BILLBOARD);

				// Render the damn mesh!
				if (FAILED(pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,
					0,
					ParticlesToRender * 2)))
				{
					WriteToLog("<b>DrawPrimitive</b> failed!");
				}

			// End the scene
			pDevice->EndScene();
/*
			// restore the world transform
			pDevice->SetTransform(D3DTS_WORLD , &w);
*/

			ParticlesRendered += ParticlesToRender;
			ParticlesToRender  = 0;
		}

	}

	return 1;
}

void MeshBillboard::Release()
{
	// free memory
	if (ParticleList) delete ParticleList;
	if (pVB) pVB->Release();
}

void MeshBillboard::ClearTransform()
{
	D3DXMatrixIdentity(&Transform);
}

void MeshBillboard::SetTransform(TRANSFORM_TYPE t , float par1,float par2,float par3)
{
	D3DXMATRIXA16 mat;
	switch (t)
	{
		case TRANSFORM_TRANSLATE:
			D3DXMatrixTranslation(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
		case TRANSFORM_ROTATE:
			D3DXMatrixRotationYawPitchRoll(&mat , par2,par1,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;

		case TRANSFORM_SCALE:
			D3DXMatrixScaling(&mat , par1,par2,par3);
			D3DXMatrixMultiply(&Transform , &Transform , &mat);
			break;
	}
}


unsigned long MeshBillboard::NumActiveParticles()
{
	unsigned long sum=0;
	for (unsigned long i=0;i<NumParticles;i++)
	{
		if (ParticleList[i].alive)
		{
			sum++;
		}
	}

	return sum;
}

void MeshBillboard::CreateFrom(PDIRECT3DDEVICE9 pDevice , Mesh3DS* pMesh)
{

	Init(pDevice , pMesh->NumVertices);

	// Copy the vertices as particles
	for (unsigned long i=0;i<NumParticles;i++)
	{
		ParticleList[i].x = pMesh->VertexList[i].x;
		ParticleList[i].y = pMesh->VertexList[i].y;
		ParticleList[i].z = pMesh->VertexList[i].z;

		ParticleList[i].size = 50.0f;
		ParticleList[i].color = 0xffffffff;
		ParticleList[i].alive = true;
	}

}

void MeshBillboard::UpdateFrom(Mesh3DS* pMesh)
{


	// Fill the vb
	MODEL_VERTEX* pVertices;
	// Lock the vb
	pMesh->pVB->Lock(0,0, (void**) &pVertices , 0);

		// Copy the vertices as particles
		for (unsigned long i=0;i<NumParticles;i++)
		{
			ParticleList[i].x = pVertices[i].x;
			ParticleList[i].y = pVertices[i].y;
			ParticleList[i].z = pVertices[i].z;
		}

	// Unlock VB
	pMesh->pVB->Unlock();

}
////////////////////////////////////////////////////////////////////

