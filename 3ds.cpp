// 3ds.cpp
// Helper functions for 3ds files

#include "3ds.h"
#include "log.h"
#include <cstdint>

/**------------------------------------------------------*/
/* Function: SkipBytes()                                 */
/*-------------------------------------------------------*/
/* Summary: Moves the file pointer of a file to the      */
/* specified number of bytes                             */
/**------------------------------------------------------*/
/* Usage: Specify byte number and a file pointer and you */
/* are set.                                              */
/**------------------------------------------------------*/
/* Return value: 1 success                               */
/*               0 at failure                            */
/**------------------------------------------------------*/
int SkipBytes(unsigned long num_of_bytes , FILE* f)
{

	for (unsigned long index = 0;index<num_of_bytes;index++)
	{
		if (feof(f))return 0;
		getc(f);
	}

	return 1;

}


/**------------------------------------------------------*/
/* Function: GoToChunk()                                 */
/*-------------------------------------------------------*/
/* Summary: Moves the file pointer of a 3ds file to the  */
/* specified chunk                                       */
/**------------------------------------------------------*/
/* Usage: Specify a chunk-id and a file pointer and you  */
/* are set.                                              */
/* CAUTION: Before using this be sure that:              */
/*          1> The file pointer currently poins at a     */
/*             chunk-id                                  */
/*          2> The file pointer points INSIDE the parent */
/*             chunk of the one you are interested in    */
/**------------------------------------------------------*/
/* Return value: Chunk's length at success               */
/*               0 at failure                            */
/**------------------------------------------------------*/
int GoToChunk(unsigned short c , FILE* f)
{
	unsigned short chunk_id;
	std::uint32_t chunk_length;

	// Read the next chunks id
	fread(&chunk_id , sizeof(unsigned short) , 1, f);

	// Loop until you find the correct chunk
	while (chunk_id != c)
	{

		// Read the number of bytes to jump
		fread(&chunk_length , sizeof(chunk_length) , 1 , f);

		// Skip this chunk
		if (!SkipBytes(chunk_length-6 , f)) return 0;

		// get the new chunks id
		fread(&chunk_id , sizeof(unsigned short) , 1 , f);

	}

	// Get the chunk's size
	fread(&chunk_length , sizeof(chunk_length) , 1 , f);

	return chunk_length;
}


/**------------------------------------------------------*/
/* Function: ExportObject()                              */
/*-------------------------------------------------------*/
/* Summary: Exports an object from a 3ds file to a s3d   */
/* file                                                  */
/**------------------------------------------------------*/
/* Usage: Specify a filename and a file pointer and you  */
/* are set.                                              */
/* CAUTION: Before using this be sure that:              */
/*          1> The file pointer currently poins at the   */
/*             object's chunk , right after the objects  */
/*             name                                      */
/**------------------------------------------------------*/
/* Return value: 1 at Success                            */
/*               0 at failure                            */
/**------------------------------------------------------*/
int ExportObject(char* filename , FILE* f)
{

	// Go to the triangular mesh chunk
	if (!GoToChunk(CHUNK_TRIANGULARMESH , f))
		return 0;

	// Go to the vertices list chunk
	if (!GoToChunk(CHUNK_VERTICESLIST , f))
		return 0;

	// Load the vertices
	unsigned short num_of_vertices;
	fread(&num_of_vertices , sizeof(unsigned short) , 1 , f);

	// Allocate space for the vertices
	COMMON_VERTEX* pVertices = (COMMON_VERTEX*) calloc(num_of_vertices, sizeof(COMMON_VERTEX));

	// Copy the vertices
	for (unsigned int index = 0;index<num_of_vertices;index++)
	{
		float temp;
		fread(&temp , sizeof(float) , 1 , f);
		pVertices[index].x = temp;
		fread(&temp , sizeof(float) , 1 , f);
		pVertices[index].y = temp;
		fread(&temp , sizeof(float) , 1 , f);
		pVertices[index].z = temp;
	}

	// Go to the faces description chunk
	if (!GoToChunk(CHUNK_FACESDESCRIPTION , f))
		return 0;

	// Get the polygon number
	unsigned short num_of_triangles;
	fread(&num_of_triangles , sizeof(unsigned short) , 1 , f);

	// Allocate memory for the triangles
	unsigned long num_of_indices = 3*num_of_triangles;
	unsigned short *pIndices = (unsigned short*) malloc(num_of_indices*sizeof(unsigned short));

	// Copy the indices
	unsigned short temp_short;
	for (unsigned int index = 0;index<num_of_triangles;index++)
	{
		// Read the 3 vertices of the face
		fread(&temp_short , sizeof(unsigned short) , 1 , f);
		pIndices[3*index] = temp_short;
		fread(&temp_short , sizeof(unsigned short) , 1 , f);
		pIndices[3*index+1] = temp_short;
		fread(&temp_short , sizeof(unsigned short) , 1 , f);
		pIndices[3*index+2] = temp_short;

		// Read and discard face material
		fread(&temp_short , sizeof(unsigned short) , 1 , f);
	}


	// Now write the data to another more-readable file
	FILE* output = fopen(filename , "w+b");

	// First write the number of vertices
	fwrite(&num_of_vertices , sizeof(unsigned short) , 1 , output);

	// Then the vertex data
	fwrite(pVertices , sizeof(COMMON_VERTEX) , num_of_vertices , output);

	// Then the number of indices
	fwrite(&num_of_indices , sizeof(unsigned short) , 1 , output);

	// Finally the indices
	fwrite(pIndices , sizeof(unsigned short) , num_of_indices, output);

	// Close the file
	fclose(output);

	// Free the memory allocated for vertices and indices
	free(pVertices);
	free(pIndices);

	return 1;
}


/**------------------------------------------------------*/
/* Function: ReadColorChunk()                            */
/*-------------------------------------------------------*/
/* Summary: Reads a color chunk's r,g and b values always*/
/* as floats                                             */
/**------------------------------------------------------*/
/* Usage: Call this function when your fp is in the      */
/* beginning of a color chunk                            */
/**------------------------------------------------------*/
/* Return value: 1 at Success                            */
/*               0 at failure                            */
/**------------------------------------------------------*/
int ReadColorChunk(float* r , float* g , float* b , FILE* f)
{

	// First see what chunk this is
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , f);
	std::uint32_t chunk_length;
	fread(&chunk_length , sizeof(chunk_length) , 1 , f);

	unsigned char cr,cg,cb;
	float fr,fg,fb;

	switch (chunk_id)
	{
		case CHUNK_COLOR_BYTE:
			fread(&cr , sizeof(unsigned char) , 1, f);
			fread(&cg , sizeof(unsigned char) , 1, f);
			fread(&cb , sizeof(unsigned char) , 1, f);
			// Convert to floats
			fr = (float) cr / 255.0f;
			fg = (float) cg / 255.0f;
			fb = (float) cb / 255.0f;
			break;

		case CHUNK_COLOR_FLOAT:
			fread(&fr , sizeof(float) , 1 , f);
			fread(&fg , sizeof(float) , 1 , f);
			fread(&fb , sizeof(float) , 1 , f);
			break;

		case CHUNK_COLOR_GBYTE:
			fread(&cr , sizeof(unsigned char) , 1, f);
			fread(&cg , sizeof(unsigned char) , 1, f);
			fread(&cb , sizeof(unsigned char) , 1, f);
			// Convert to floats
			fr = (float) cr / 255.0f;
			fg = (float) cg / 255.0f;
			fb = (float) cb / 255.0f;
			break;

		case CHUNK_COLOR_GFLOAT:
			fread(&fr , sizeof(float) , 1 , f);
			fread(&fg , sizeof(float) , 1 , f);
			fread(&fb , sizeof(float) , 1 , f);
			break;

		default:
			return 0;
			break;
	}

	// return the values
	*r = fr;
	*g = fg;
	*b = fb;

	// return success
	return 1;

}


/**------------------------------------------------------*/
/* Function: ReadPercentChunk()                          */
/*-------------------------------------------------------*/
/* Summary: Reads a percent chunk always                 */
/* as a float                                            */
/**------------------------------------------------------*/
/* Usage: Call this function when your fp is in the      */
/* beginning of a percent chunk                          */
/**------------------------------------------------------*/
/* Return value: 1 at Success                            */
/*               0 at failure                            */
/**------------------------------------------------------*/
int ReadPercentChunk(float* p , FILE* f)
{

	// First see what chunk this is
	unsigned short chunk_id;
	fread(&chunk_id , sizeof(unsigned short) , 1 , f);
	std::uint32_t chunk_length;
	fread(&chunk_length , sizeof(chunk_length) , 1 , f);

	unsigned short ip;
	float fp;

	switch (chunk_id)
	{
		case CHUNK_PERCENT_INT:
			fread(&ip , sizeof(unsigned short) , 1, f);
			// Convert to float
			fp = (float) ip;
			break;

		case CHUNK_PERCENT_FLOAT:
			fread(&fp , sizeof(float) , 1, f);
			break;

		default:
			return 0;
			break;
	}

	// return the value
	*p = fp;

	// return success
	return 1;

}


/**------------------------------------------------------*/
/* Function: SearchForChunkUntil()                       */
/*-------------------------------------------------------*/
/* Summary: Searches for the specified chunk until it    */
/* another type of chunk . If it encounters the last ,   */
/* it stops.For limitations see GoToChunk()		         */
/**------------------------------------------------------*/
/* Usage: Call this function when your fp is in the      */
/* beginning of a chunk that is in the same superchunk   */
/* as the chunks you are interested in					 */
/**------------------------------------------------------*/
/* Return value: chunk size when it finds the specified  */
/*               0 if it encounters the "until" chunk    */
/**------------------------------------------------------*/
int SearchForChunkUntil(unsigned short c , unsigned short until, FILE*f)
{
	unsigned short chunk_id;
	std::uint32_t chunk_length;

	// Read the next chunks id
	fread(&chunk_id , sizeof(unsigned short) , 1, f);

	// Loop until you find the correct chunk
	while (chunk_id != c)
	{

		// Read the number of bytes to jump
		fread(&chunk_length , sizeof(chunk_length) , 1 , f);

		if (chunk_id == until)
		{
			return 0;
		}

		// Skip this chunk
		if (!SkipBytes(chunk_length-6 , f)) return 0;

		// get the new chunks id
		fread(&chunk_id , sizeof(unsigned short) , 1 , f);

	}

	// Get the chunk's size
	fread(&chunk_length , sizeof(chunk_length) , 1 , f);

	return chunk_length;
}
