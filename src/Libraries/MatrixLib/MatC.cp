/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: MatC.c
    Created: Saturday, October 17, 1992, 1:57
    Modified: Wednesday, November 29, 1995, 1:58
/*/

#include "FastMat.h"
#include <FixMath.h>
#include <math.h>

/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication
**	if n is 1 and a matrix with matrix multiplication when
**	n is 4.
*/
void	VectorMatrixProductC(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	int		j,k;
		
	while(n--)
	{	for(j=0;j<4;j++)
		{	(*vd)[j]=0;
			for(k=0;k<4;k++)
			{	(*vd)[j]+=FixMul((*vs)[k],(*m)[k][j]);
			}
		}
		vd++;
		vs++;
	}
}

Fixed	FMulC(
	Fixed	a,
	Fixed	b)
{
	return	FixMul(a,b);
}

Fixed	FDivC(
	Fixed	a,
	Fixed	b)
{
	return	FixDiv(a,b);
}

Fixed	FMulDivC(
	Fixed	a,
	Fixed	b,
	Fixed	c)
{
	return (long) (((double)a) * b / c);
}

#ifdef CTRIGONOMETRY
Fixed	FRadSin(
	Fixed	a)
{
	return (long)(65536L*sin(a/65536.0));
}

Fixed	FRadCos(
	Fixed	a)
{
	return (long)(65536L*cos(a/65536.0));
}

Fixed	FDegSin(
	Fixed	a)
{
	return	(long)(65536.0*sin(a/3754936.206169363));
}

Fixed	FDegCos(
	Fixed	a)
{
	return	(long)(65536.0*cos(a/3754936.206169363));
}

Fixed	FOneSin(
	Fixed	a)
{
	return	(long)(65536.0*sin(a/10430.3783505));
}

Fixed	FOneCos(
	Fixed	a)
{
	return	(long)(65536.0*cos(a/10430.3783505));
}
#endif
void	Transpose(
	Matrix	*a)
{
	int		i,j;
	Fixed	temp;
	
	for(i=0;i<4;i++)
	{	for(j=i+1;j<4;j++)
		{	temp = (*a)[i][j];
			(*a)[i][j] = (*a)[j][i];
			(*a)[j][i] = temp;
		}
	}
}

Fixed	FSqrt(
	Fixed	n)
{
	short	temp[4];
	
	temp[0] = 0;
	temp[3] = 0;
	*(long *)(temp+1) = n;
	return FSqroot((long *)temp);
}

#define	RANDCONST	((unsigned long)(0x41A7))
#define	HIGH(x)		((unsigned int)(x>>16))
#define	LOW(x)		((unsigned int)x)

Fixed		FRandomC()
{
	register	unsigned	long	temp;
	
	temp = RANDCONST * HIGH(FRandSeed) + HIGH(RANDCONST * LOW(FRandSeed));
	FRandSeed = ((temp & 0x7fff) << 16) + HIGH(temp << 1) + LOW(RANDCONST * FRandSeed);
	
	return	LOW(FRandSeed);
}

/*
**	Move by xt, yt, zt
*/
void	MTranslate(
	Fixed	xt,
	Fixed	yt,
	Fixed	zt,
	Matrix	*theMatrix)
{
	(*theMatrix)[3][0] += xt;
	(*theMatrix)[3][1] += yt;
	(*theMatrix)[3][2] += zt;
}

void	MRotateX(
	Fixed	s,
	Fixed	c,
	Matrix	*theMatrix)
{
	short	i;
	Fixed	*vm;
	Fixed	*em;
	
	vm = &(*theMatrix)[0][0];
	em = vm + 16;
	
	do
	{	Fixed	t = vm[1];
		
		vm[1] = FMul(t, c) - FMul(vm[2], s);
		vm[2] = FMul(t, s) + FMul(vm[2], c);
		vm += 4;
	} while(vm < em);
}

void	MRotateY(
	Fixed	s,
	Fixed	c,
	Matrix	*theMatrix)
{
	short	i;
		
	for(i=0;i<4;i++)
	{	Fixed	t = (*theMatrix)[i][0];
		
		(*theMatrix)[i][0] = FMul(t, c) + FMul((*theMatrix)[i][2], s);
		(*theMatrix)[i][2] = FMul((*theMatrix)[i][2], c) - FMul(t, s);
	}
}

void	MRotateZ(
	Fixed	s,
	Fixed	c,
	Matrix	*theMatrix)
{
	short	i;
		
	for(i=0;i<4;i++)
	{	Fixed	t = (*theMatrix)[i][0];
		
		(*theMatrix)[i][0] = FMul(t, c) - FMul((*theMatrix)[i][1], s);
		(*theMatrix)[i][1] = FMul(t, s) + FMul((*theMatrix)[i][1], c);
	}
}


/*
**	If "t" is a matrix that only does translates and rotates,
**	the inverse is very easy to find. Find that inverse and
**	return the result in "i".
*/
void	InverseTransform(
register	Matrix	*t,
register	Matrix	*i)
{
	Vector	temp;

	/*	Transpose 3x3 matrix
	*/
	(*i)[0][0] = (*t)[0][0];	(*i)[0][1] = (*t)[1][0];	(*i)[0][2] = (*t)[2][0];
	(*i)[1][0] = (*t)[0][1];	(*i)[1][1] = (*t)[1][1];	(*i)[1][2] = (*t)[2][1];
	(*i)[2][0] = (*t)[0][2];	(*i)[2][1] = (*t)[1][2];	(*i)[2][2] = (*t)[2][2];

	(*i)[0][3] = (*i)[1][3] = (*i)[2][3] = 0;		(*i)[3][3] = FIX(1);
	(*i)[3][0] = (*i)[3][1] = (*i)[3][2] = 0;

	/*	Find translate part of matrix:
	*/
	VectorMatrix33Product(1, &((*t)[3]), &temp, i);
	(*i)[3][0] = -temp[0];
	(*i)[3][1] = -temp[1];
	(*i)[3][2] = -temp[2];
}

/*
**	Converts a unit quaternion into a rotation matrix.
**	Algorithm according to Ken Shoemake, SIGGraph-85
**	"Animating Rotation with Quaternion Curves"
*/
void	QuaternionToMatrix(
	Quaternion	*q,
	Matrix		*m)
{
	Fixed	x2,y2,z2;
	Fixed	wx2,yx2,zx2,xx2;
	Fixed	wy2,yy2,zy2;
	Fixed	wz2,zz2;
	
	x2 = q->x << 1;
	y2 = q->y << 1;
	z2 = q->z << 1;
	
	wx2 = FMul(x2,q->w);	xx2 = FMul(x2,q->x);
	yx2 = FMul(x2,q->y);	zx2 = FMul(x2,q->z);

	wy2 = FMul(y2,q->w);
	yy2 = FMul(y2,q->y);	zy2 = FMul(y2,q->z);
	
	wz2 = FMul(z2,q->w);
	zz2 = FMul(z2,q->z);
	
	(*m)[0][0] = FIX(1) - yy2 - zz2;
	(*m)[1][1] = FIX(1) - xx2 - zz2;
	(*m)[2][2] = FIX(1) - xx2 - yy2;
	
	(*m)[0][1] = yx2 + wz2;
	(*m)[0][2] = zx2 - wy2;
	(*m)[1][0] = yx2 - wz2;
	(*m)[1][2] = zy2 + wx2;
	(*m)[2][0] = zx2 + wy2;
	(*m)[2][1] = zy2 - wx2;
}

#define	EPSILON	4

/*
**	Convert a rotation matrix (really 3x3) into
**	quaternion format.
**
**	Algorithm according to Ken Shoemake, SIGGraph-85
**	"Animating Rotation with Quaternion Curves"
*/
void	MatrixToQuaternion(
	Matrix		*m,
	Quaternion	*q)
{
	Fixed	squared;
	
	squared = 4 * (FIX(1) + (*m)[0][0] + (*m)[1][1] + (*m)[2][2]);
	
	if(squared > EPSILON)
	{	q->w = FSqrt(squared);
		q->x = FDiv((*m)[1][2] - (*m)[2][1] , q->w);
		q->y = FDiv((*m)[2][0] - (*m)[0][2] , q->w);
		q->z = FDiv((*m)[0][1] - (*m)[1][0] , q->w);
		
		q->w /= 4;
	}
	else
	{
		q->w = 0;
		squared = -(*m)[1][1]-(*m)[2][2];
		squared += squared;
		
		if(squared > EPSILON)
		{	q->x = FSqrt(squared);
			q->y = FDiv( (*m)[0][1] , q->x);
			q->z = FDiv( (*m)[0][2] , q->x);
			q->x /= 2;
		}
		else
		{	q->x = 0;
			squared = FIX(1) - (*m)[2][2];
			
			if(squared > EPSILON)
			{	q->y = FSqrt(squared);
				q->z = FDiv( (*m)[1][2], q->y);
				q->y /= 2;
			}
			else
			{	q->y = 0;
				q->z = FIX(1);
			}
		}
	}
}
