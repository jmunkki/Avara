/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: DoubleMat.c
    Created: Thursday, May 4, 1995, 11:25
    Modified: Sunday, November 19, 1995, 8:26
/*/

#include "FastMat.h"

#define	CALCTYPE	float

static	CALCTYPE	toFixedConstant = 1/65536.;

void	FlaggedVectorMatrix34ProductC(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	CALCTYPE	m00,m01,m02;
	CALCTYPE	m10,m11,m12;
	CALCTYPE	m20,m21,m22;
	Fixed		m30,m31,m32;
	CALCTYPE	x,y,z;
	Fixed		*p;
	
	if(n)
	{
#ifdef OLDCODE
		p = &(*m)[0][0];

		m00 = *p++;		m01 = *p++;		m02 = *p++;		p++;
		m10 = *p++;		m11 = *p++;		m12 = *p++;		p++;
		m20 = *p++;		m21 = *p++;		m22 = *p++;		p++;
		m30 = *p++;		m31 = *p++;		m32 = *p++;
#else
		m00 = (*m)[0][0] * toFixedConstant;
		m01 = (*m)[0][1] * toFixedConstant;
		m02 = (*m)[0][2] * toFixedConstant;

		m10 = (*m)[1][0] * toFixedConstant;
		m11 = (*m)[1][1] * toFixedConstant;
		m12 = (*m)[1][2] * toFixedConstant;

		m20 = (*m)[2][0] * toFixedConstant;
		m21 = (*m)[2][1] * toFixedConstant;
		m22 = (*m)[2][2] * toFixedConstant;

		m30 = (*m)[3][0];	m31 = (*m)[3][1];	m32 = (*m)[3][2];
#endif		
		do
		{	if((*vs)[3])
			{	x = (*vs)[0];
				y = (*vs)[1];
				z = (*vs)[2];
			
				(*vd)[0] = m30 + (long)(m20 * z + m10 * y + m00 * x);
				(*vd)[1] = m31 + (long)(m21 * z + m11 * y + m01 * x);
				(*vd)[2] = m32 + (long)(m22 * z + m12 * y + m02 * x);
			}
			vs++;
			vd++;
		} while(--n);
	}
}

typedef	float	FloatVect[4];

void	FlaggedVectorMatrix34ProductF(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	FloatVect	*vdf;
	CALCTYPE	m00,m01,m02;
	CALCTYPE	m10,m11,m12;
	CALCTYPE	m20,m21,m22;
	CALCTYPE	m30,m31,m32;
	CALCTYPE	x,y,z;
	Fixed		*p;
	
	if(n)
	{
		vdf = (FloatVect *)vd;

		m00 = (*m)[0][0] * toFixedConstant;
		m01 = (*m)[0][1] * toFixedConstant;
		m02 = (*m)[0][2] * toFixedConstant;

		m10 = (*m)[1][0] * toFixedConstant;
		m11 = (*m)[1][1] * toFixedConstant;
		m12 = (*m)[1][2] * toFixedConstant;

		m20 = (*m)[2][0] * toFixedConstant;
		m21 = (*m)[2][1] * toFixedConstant;
		m22 = (*m)[2][2] * toFixedConstant;

		m30 = (*m)[3][0];	m31 = (*m)[3][1];	m32 = (*m)[3][2];

		do
		{	if((*vs)[3])
			{	x = (*vs)[0];
				y = (*vs)[1];
				z = (*vs)[2];
			
				(*vdf)[0] = m30 + m20 * z + m10 * y + m00 * x;
				(*vdf)[1] = m31 + m21 * z + m11 * y + m01 * x;
				(*vdf)[2] = m32 + m22 * z + m12 * y + m02 * x;
			}
			vs++;
			vdf++;
		} while(--n);
	}
}