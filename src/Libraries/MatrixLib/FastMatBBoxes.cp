/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: FastMatBBoxes.c
    Created: Saturday, November 11, 1995, 6:23
    Modified: Thursday, November 23, 1995, 15:11
/*/

#include "FastMat.h"

/*
	Given a matrix and minimum and maximum coordinates to define an axis-aligned
	box, the box is transformed and the eight corner vertices are stored in dest.
	
	To do this, six vectors have to be transformed:
	
	left	minX	0		0		->	(minX * m[0][0], minX * m[1][0], minX * m[2][0])
	right	maxX	0		0		->	(maxX * m[0][0], maxX * m[1][0], maxX * m[2][0])

	bottom	0		minY	0		->	(minY * m[0][1], minY * m[1][1], minY * m[2][1])
	top		0		maxY	0		->	(maxY * m[0][1], maxY * m[1][1], maxY * m[2][1])

	front	0		0		minZ	->	(minZ * m[0][2], minZ * m[1][2], minZ * m[2][2])
	back	0		0		maxZ	->	(maxZ * m[0][2], maxZ * m[1][2], maxZ * m[2][2])

	These are added in different combinations with the bottom row of the matrix
	(which defines the new origin) to produce the eight different corners.
*/

asm
void	TransformMinMax(
			Matrix	*m,
			Fixed	*min,
			Fixed	*max,
			Vector	*dest)
{
	machine	68020
	fralloc
	
	movem.l	D3-D6/A2-A3,-(sp)
	move.l	min, A0
	move.l	max, A1
	move.l	dest,A2
	move.l	m, A3

	moveq.l	#2,D0		//	Loop through three dimensions (x,y,z)
@loopy:
	move.l	(A0)+,D1	//	min bound to D1
	move.l	(A3)+,D2	//	matrix[row][0]	to	D2

	move.l	D1,D5		//	copy min

	muls.l	D2,D6:D5	//	multiply
	move.l	(A3)+,D3	//	matrix[row][1]	to	D3
	move.w	D6,D5
	swap	D5			//	result in D5
	move.l	D5,(A2)+	//	Store in components

	move.l	D1,D5		//	copy min
	muls.l	D3,D6:D5	//	multiply
	move.l	(A3),D4		//	matrix[row][2]	to	D4
	move.w	D6,D5

	muls.l	D4,D6:D1	//	multiply
	swap	D5			//	result in D5
	move.l	D5,(A2)+	//	Store in components
	move.w	D6,D1
	swap	D1			//	result in D1
	move.l	D1,(A2)		//	Store in components
	addq.l	#8,A3

	move.l	(A1)+,D1	//	max bound to D1
	addq.l	#8,A2

	move.l	D1,D5		//	copy max
	muls.l	D2,D6:D5	//	multiply
	move.w	D6,D5
	swap	D5			//	result in D5
	move.l	D5,(A2)+	//	Store in components

	move.l	D1,D5		//	copy max
	muls.l	D3,D6:D5	//	multiply
	move.w	D6,D5

	muls.l	D4,D6:D1	//	multiply
	swap	D5			//	result in D5
	move.l	D5,(A2)+	//	Store in components
	move.w	D6,D1
	swap	D1			//	result in D1
	move.l	D1,(A2)		//	Store in components
	addq.l	#8,A2

	dbra	D0,@loopy
	movem.l	(sp)+,D3-D6/A2-A3
	
	frfree
	rts
}

void	TransformBoundingBox(
register	Matrix	*m,
			Fixed	*min,
			Fixed	*max,
			Vector	*dest)
{
	register	Fixed	*dp;
	register	Fixed	f1 = FIX(1);
	register	Fixed	x,y,z;
				Vector	comp[6];

	TransformMinMax(m, min, max, comp);
	x = (*m)[3][0];
	y = (*m)[3][1];
	z = (*m)[3][2];

/*	The first part is now done and we have the six vectors transformed.
**	The rest is easy enough to with C.
*/
#define	COMPCOPY(i,a,b,c,d)	*dp++	= d + comp[a][i] + comp[b+2][i] + comp[c+4][i];
#define	ALLCOMP(a,b,c)	COMPCOPY(0,a,b,c,x)	\
						COMPCOPY(1,a,b,c,y)	\
						COMPCOPY(2,a,b,c,z)	\
						*dp++ = f1;
	dp = dest[0];

	ALLCOMP(0,0,0);
	ALLCOMP(1,1,1);
	ALLCOMP(0,0,1);
	ALLCOMP(1,1,0);
	ALLCOMP(0,1,0);
	ALLCOMP(1,0,1);
	ALLCOMP(1,0,0);
	ALLCOMP(0,1,1);
}
