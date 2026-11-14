// camera.cpp

#include "camera.h"
#include <stdio.h>
#include "log.h"

// GLOBALS ////////////////////////////////////

D3DXMATRIXA16 view_matrix;

// FUNCTIONS //////////////////////////////////

CameraShot::CameraShot()
{

	x=0;
	y=0;
	z=0;

	yaw=0;
	pitch=0;
	roll=0;

}

///////////////////////////////////////////////

CameraShot::CameraShot(float X   , float Y     , float Z , 
					   float YAW , float PITCH , float ROLL)
{

	x=X;
	y=Y;
	z=Z;

	yaw=YAW;
	pitch=PITCH;
	roll=ROLL;

}

void CameraShot::Set(float X   , float Y     , float Z , 
					 float YAW , float PITCH , float ROLL)
{

	x=X;
	y=Y;
	z=Z;

	yaw=YAW;
	pitch=PITCH;
	roll=ROLL;

}

void CameraShot::Add(float X   , float Y     , float Z , 
					 float YAW , float PITCH , float ROLL)
{

	x+=X;
	y+=Y;
	z+=Z;

	yaw+=YAW;
	pitch+=PITCH;
	roll+=ROLL;

}

//////////////////////////////////////////////

void CameraShot::UpdateViewMatrix(IDirect3DDevice9* pDevice)
{

	D3DXVECTOR3 position,up,right,look;
	position.x = x;
	position.y = y;
	position.z = z;

	up.x = 0; up.y = 1; up.z=0;
	right.x=1; right.y=0; right.z=0;
	look.x=0; look.y=0; look.z=1;

	// craete a rotation matrix with yaw pitch and roll
	D3DXMATRIXA16 matRot;
	D3DXMatrixRotationYawPitchRoll(&matRot , yaw , pitch , roll);

	// Multiply the three vectors with that matrix
	D3DXVec3TransformCoord(&up,&up,&matRot);
	D3DXVec3TransformCoord(&right,&right,&matRot);
	D3DXVec3TransformCoord(&look,&look,&matRot);

	// make the view matrix
	D3DXMATRIXA16 matView;
	D3DXMatrixIdentity(&matView);

	matView._11 = right.x;
	matView._21 = right.y;
	matView._31 = right.z;

	matView._12 = up.x;
	matView._22 = up.y;
	matView._32 = up.z;

	matView._13 = look.x;
	matView._23 = look.y;
	matView._33 = look.z;

	matView._41 = - D3DXVec3Dot(&right , &position);
	matView._42 = - D3DXVec3Dot(&up , &position);
	matView._43 = - D3DXVec3Dot(&look , &position);

	// Update the view matrix please!
	pDevice->SetTransform( D3DTS_VIEW, &matView );

	/*

	// yaw (y) and pitch (x) affect the forward vector
	D3DXVECTOR3 forward;
	forward.z = (float)cos(pitch);	// pitch for forward vector	
	forward.y = (float)sin(pitch);

	forward.x = forward.z * (float)sin(yaw);// yaw for forward vector
	forward.z = forward.z * (float)cos(yaw);

	// pitch (x) and roll (z) affect the up vector
	D3DXVECTOR3 up;
	up.y = (float)cos(roll);	// roll for up vector
	up.x = (float)sin(roll);

	up.z = up.y * (float)sin(pitch);		// pitch for up vector
	up.y = up.y * (float)cos(pitch);

	// now that we have the position , the up vector 
	// and the forward looking vector , we need a view
	// point . We get this by adding the position vector
	// to the forward vector
	forward += position;

	// Update the view matrix please!
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &position, &forward, &up );
	pDevice->SetTransform( D3DTS_VIEW, &matView );

  */


}

D3DXMATRIXA16* CameraShot::ProduceViewMatrix()
{

	D3DXVECTOR3 position,up,right,look;
	position.x = x;
	position.y = y;
	position.z = z;

	up.x = 0; up.y = 1; up.z=0;
	right.x=1; right.y=0; right.z=0;
	look.x=0; look.y=0; look.z=1;

	// craete a rotation matrix with yaw pitch and roll
	D3DXMATRIXA16 matRot;
	D3DXMatrixRotationYawPitchRoll(&matRot , yaw , pitch , roll);

	// Multiply the three vectors with that matrix
	D3DXVec3TransformCoord(&up,&up,&matRot);
	D3DXVec3TransformCoord(&right,&right,&matRot);
	D3DXVec3TransformCoord(&look,&look,&matRot);

	D3DXMatrixIdentity(&view_matrix);

	view_matrix._11 = right.x;
	view_matrix._21 = right.y;
	view_matrix._31 = right.z;

	view_matrix._12 = up.x;
	view_matrix._22 = up.y;
	view_matrix._32 = up.z;

	view_matrix._13 = look.x;
	view_matrix._23 = look.y;
	view_matrix._33 = look.z;

	view_matrix._41 = - D3DXVec3Dot(&right , &position);
	view_matrix._42 = - D3DXVec3Dot(&up , &position);
	view_matrix._43 = - D3DXVec3Dot(&look , &position);

	// Return the matrix
	return &view_matrix;
}

D3DXVECTOR3 CameraShot::GetDirection()
{

	D3DXVECTOR3 look;

	look.x=0; look.y=0; look.z=1;

	// craete a rotation matrix with yaw pitch and roll
	D3DXMATRIXA16 matRot;
	D3DXMatrixRotationYawPitchRoll(&matRot , yaw , pitch , roll);

	// Multiply the three vectors with that matrix
	D3DXVec3TransformCoord(&look,&look,&matRot);

	return look;
}

//////////////////////////////////////////////

void CameraShot::Update(CAMERA_UPDATE_TYPE type , 
		                float par1 , float par2)
{

	// Init the camera's coord system
	D3DXVECTOR3 position,up,right,look;
	position.x = x;
	position.y = y;
	position.z = z;

	up.x = 0; up.y = 1; up.z=0;
	right.x=1; right.y=0; right.z=0;
	look.x=0; look.y=0; look.z=1;

	// craete a rotation matrix with yaw pitch and roll
	D3DXMATRIXA16 matRot;
	D3DXMatrixRotationYawPitchRoll(&matRot , yaw , pitch , roll);

	// Multiply the three vectors with that matrix
	D3DXVec3TransformCoord(&up,&up,&matRot);
	D3DXVec3TransformCoord(&right,&right,&matRot);
	D3DXVec3TransformCoord(&look,&look,&matRot);


	// There are 3 types of update.

	// Position translates the camera in the XZ plane
	// of the CAMERA's OWN COORDINATE SYSTEM
	if (type == UPDATE_POSITION)
	{
		// We can translate the camera
		// Using its coord system

		// Translate in the right direction
		position += right * par1 * 400.0f;
		// Translate in the forward direction
		position -= look * par2 * 400.0f;

	}

	// YAW_PITCH changes the yaw and pitch of the camera
	if (type == UPDATE_YAW_PITCH)
	{
	
		// we dont need a coordinate system here , just
		// change the yaw and pitch values
		yaw += par1;
		pitch += par2;

	}

	// ROLL_HEIGHT changes the roll and height of the camera
	if (type == UPDATE_ROLL_HEIGHT)
	{
		// Now we need just the up vector

		// update position
		position -= up * par2 * 400.0f;

		// update roll
		roll -= par1;

	}

	// Update the position
	x = position.x;
	y = position.y;
	z = position.z;
	

}

void CameraShot::Cerp(CameraShot* c0,CameraShot* c1,
		CameraShot* c2,CameraShot* c3,float t)
{

	float t2,t3;
	t2 = t*t;
	t3 = t2*t;

	float P , Q , R , S;

	// find the polynomial
	P = (c3->x - c2->x) - (c0->x - c1->x);
	Q = (c0->x - c1->x) - P;
	R = c2->x - c0->x;
	S = c1->x;
	x = P*t3 + Q*t2 + R*t + S;

	// find the polynomial
	P = (c3->y - c2->y) - (c0->y - c1->y);
	Q = (c0->y - c1->y) - P;
	R = c2->y - c0->y;
	S = c1->y;
	y = P*t3 + Q*t2 + R*t + S;

	// find the polynomial
	P = (c3->z - c2->z) - (c0->z - c1->z);
	Q = (c0->z - c1->z) - P;
	R = c2->z - c0->z;
	S = c1->z;
	z = P*t3 + Q*t2 + R*t + S;

	// find the polynomial
	P = (c3->yaw - c2->yaw) - (c0->yaw - c1->yaw);
	Q = (c0->yaw - c1->yaw) - P;
	R = c2->yaw - c0->yaw;
	S = c1->yaw;
	yaw = P*t3 + Q*t2 + R*t + S;

	// find the polynomial
	P = (c3->pitch - c2->pitch) - (c0->pitch - c1->pitch);
	Q = (c0->pitch - c1->pitch) - P;
	R = c2->pitch - c0->pitch;
	S = c1->pitch;
	pitch = P*t3 + Q*t2 + R*t + S;

	// find the polynomial
	P = (c3->roll - c2->roll) - (c0->roll - c1->roll);
	Q = (c0->roll - c1->roll) - P;
	R = c2->roll - c0->roll;
	S = c1->roll;
	roll = P*t3 + Q*t2 + R*t + S;


}

CameraSequence::CameraSequence()
{

	m_dwNumShots = 0;
	m_pKeyframes = NULL;
	m_dwTotalTime = 0;

}

CameraSequence::~CameraSequence()
{

	if (m_pKeyframes)
		delete[] m_pKeyframes;

}

bool CameraSequence::Load(char* filename)
{

	FILE* input;
	input = fopen(filename , "rb");

	if (!input)
	{
		WriteToLog("Couldn't load the camera - couldn't open the input file");
		return false;
	}

	// read in number of frames
	fread(&m_dwNumShots , sizeof(m_dwNumShots),1,input);

	if (!m_dwNumShots) 
	{
		WriteToLog("This camera file has no shots");
		return false;
	}

	// allocate space for the keyframes
	m_pKeyframes = new CameraShot[m_dwNumShots];

	// read in the shots!
	fread(m_pKeyframes , sizeof(CameraShot) , m_dwNumShots , input);

	fclose(input);

	// Compute total time
	for (unsigned long index = 0;index<m_dwNumShots;index++)
	{
		m_dwTotalTime += m_pKeyframes[index].TimeDelta;
	}

	return true;
}

void CameraSequence::FindFrames(unsigned long time)
{

	// This function finds where we are in the sequence

	unsigned long curr_time = 0;

	unsigned long index = 0;

	while ( (curr_time + m_pKeyframes[index+1].TimeDelta) <= time)
	{
		curr_time += m_pKeyframes[index+1].TimeDelta;
		index++;
	}

	// find the 4 contributing shots

	if (((long)(index)-1) >=0) c0 = index-1;
	else c0=0;

	c1 = index;
	c2 = index+1;
	if (c2>=m_dwNumShots)
		c2 = m_dwNumShots-1;
	c3 = index+2;
	if (c3>=m_dwNumShots)
		c3 = m_dwNumShots-1;

	// we have the sequence , find out the time delta
	unsigned long delta = time - curr_time;
	m_fCerp = (float) delta / (float) m_pKeyframes[index+1].TimeDelta;


}

void CameraSequence::Cerp(unsigned long time)
{

	// First find the correct frames
	FindFrames(time);
	m_CurrentView.Cerp(m_pKeyframes+c0,m_pKeyframes+c1,
		m_pKeyframes+c2,m_pKeyframes+c3,m_fCerp);

}

void CameraSequence::UpdateViewMatrix(IDirect3DDevice9* pDevice)
{

	m_CurrentView.UpdateViewMatrix(pDevice);

}

// CameraSpherical Utility
D3DXMATRIXA16* CameraSpherical(float r,float phi,float theta)
{

	D3DXVECTOR3 up,look,position;
	
	up.x = 0;
	up.y = 1;
	up.z = 0;

	look.x = 0;
	look.y = 0;
	look.z = 1;

	D3DXMATRIXA16 rotationX,rotationY,rotation;
	D3DXMatrixRotationX(&rotationX , phi);
	D3DXMatrixRotationY(&rotationY , theta);
	D3DXMatrixMultiply(&rotation , &rotationX , &rotationY);

	D3DXVec3TransformCoord(&up,&up,&rotation);
	D3DXVec3TransformCoord(&look,&look,&rotation);


	position = -look*r;

	look += position;

	D3DXMATRIXA16 matOut;

	return (D3DXMATRIXA16*)D3DXMatrixLookAtLH(&matOut , &position , &look , &up);
}