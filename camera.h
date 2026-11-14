// Camera.h
// Hehe I coded this in my sleep! Honestly!

// LOCK ////////////////////////////////////

#ifndef __CAMERA_HEADER
#define __CAMERA_HEADER

// INCLUDES ////////////////////////////////

#include "D3DX9.h"

// ENUMS ///////////////////////////////////
enum CAMERA_UPDATE_TYPE
{

	UPDATE_POSITION,
	UPDATE_YAW_PITCH,
	UPDATE_ROLL_HEIGHT

};

// CLASSES /////////////////////////////////


class CameraShot
{
// ------
// This class contains a camera keyframe
// It has methods to make a view matrix
// and upload it to the graphics card
// ------

public:
	float			x,y,z;
	float			yaw,pitch,roll;
	std::uint32_t	TimeDelta;

	CameraShot();
	CameraShot(float X   , float Y     , float Z , 
			   float YAW , float PITCH , float ROLL);

	void UpdateViewMatrix(IDirect3DDevice9* pDevice);
	void Set(float X   , float Y     , float Z , 
			 float YAW , float PITCH , float ROLL);
	void Add(float X   , float Y     , float Z , 
			 float YAW , float PITCH , float ROLL);
	void Update(CAMERA_UPDATE_TYPE type , 
		        float par1 , float par2);
	void Cerp(CameraShot* c0,CameraShot* c1,
		CameraShot* c2,CameraShot* c3,float t);
	D3DXVECTOR3 GetDirection();
	D3DXMATRIXA16* ProduceViewMatrix();

}; // End Class CameraShot

class CameraSequence

// ------
// This class contains a sequence of camera shots (keyframes)
// along with methods to interpolate for smooth animation
// ------
{
public:

	std::uint32_t		m_dwNumShots;
	CameraShot*			m_pKeyframes;

	unsigned long		m_dwTotalTime;
	unsigned long		c0,c1,c2,c3;

	CameraShot			m_CurrentView;

	float				m_fCerp;

	CameraSequence();
	~CameraSequence();

	bool Load(char* filename);
	void FindFrames(unsigned long time);
	void Cerp(unsigned long time);
	void UpdateViewMatrix(IDirect3DDevice9* pDevice);


}; // End class CameraSequence

// CameraSpherical utility
D3DXMATRIXA16* CameraSpherical(float r,float phi,float theta);

// LOCK ////////////////////////////////////

#endif