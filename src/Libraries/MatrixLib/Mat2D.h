/*/
    Copyright ©1992-1994, Juri Munkki
    All rights reserved.

    File: Mat2D.h
    Created: Thursday, December 10, 1992, 16:07
    Modified: Friday, March 11, 1994, 18:11
/*/

#pragma once

typedef	Fixed	V2D[2];	/*	A 2D vector consists of 2 fixed point numbers: x,y		*/
typedef	V2D		M2D[3];	/*	A 2x3 matrix consists of 3 vectors.						*/

typedef	struct			/*	A 2D circle can be defined like this.					*/
{	Fixed	x;
	Fixed	y;
	Fixed	r;
} FixedCircle;

void	Transform2D(	long	n,
						V2D		*vs,
						V2D		*vd,
						M2D		*m);

void	TransformCircles(long		n,
						FixedCircle	*cs,
						FixedCircle	*cd,
						M2D			*m,
						Fixed		scaleFactor);

void	MatrixProduct2D(M2D		*vs,
						M2D		*vd,
						M2D		*m);

Fixed	DistanceEstimate(	Fixed	x1,
							Fixed	y1,
							Fixed	x2,
							Fixed	y2);