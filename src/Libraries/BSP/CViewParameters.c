/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CViewParameters.c
    Created: Sunday, July 10, 1994, 21:24
    Modified: Tuesday, February 20, 1996, 7:39
/*/

#include "CViewParameters.h"
#include "FastMat.h"
#include "CBSPPart.h"
#include "PAKit.h"
#include "PolyColor.h"

#define	PYRAMIDSCALE	>>2

void	CViewParameters::CalculateViewPyramidCorners()
{
	short	i;
	Fixed	fixedDimH, fixedDimV;
	
	fixedDimH = ((long)viewPixelDimensions.h) << 16;
	fixedDimV = ((long)viewPixelDimensions.v) << 16;
	
	for(i=0;i<8;i++)	corners[i][3] = FIX(1);		//	W components for all corners.

	for(i=0;i<4;i++)	//	Z components at hither and yon.
	{	corners[i][2] = hitherBound PYRAMIDSCALE;
		corners[i+4][2] = horizonBound PYRAMIDSCALE;
	}
	
	//	X coordinates, first close ones:
	corners[0][0] = FMulDivNZ(xOffset, hitherBound, screenScale) PYRAMIDSCALE;
	corners[1][0] = corners[0][0];

	corners[2][0] = FMulDivNZ(xOffset - fixedDimH, hitherBound, screenScale) PYRAMIDSCALE;
	corners[3][0] = corners[2][0];

	//	Then X coordinates at yon
	corners[4][0] = FMulDivNZ(xOffset, horizonBound, screenScale) PYRAMIDSCALE;
	corners[5][0] = corners[4][0];

	corners[6][0] = FMulDivNZ(xOffset - fixedDimH, horizonBound, screenScale) PYRAMIDSCALE;
	corners[7][0] = corners[6][0];

	//	Then Y coordinates at hither
	corners[0][1] = FMulDivNZ(yOffset, hitherBound, screenScale) PYRAMIDSCALE;
	corners[1][1] = FMulDivNZ(yOffset - fixedDimV, hitherBound, screenScale) PYRAMIDSCALE;
	corners[2][1] = corners[0][1];
	corners[3][1] = corners[1][1];

	//	Then Y coordinates at yon
	corners[4][1] = FMulDivNZ(yOffset, horizonBound, screenScale) PYRAMIDSCALE;
	corners[5][1] = FMulDivNZ(yOffset - fixedDimV, horizonBound, screenScale) PYRAMIDSCALE;
	corners[6][1] = corners[4][1];
	corners[7][1] = corners[5][1];
}

#define	DIFFSIDES(a,b)	((a > b) ? (a > altitude && b <= altitude) : (b > altitude && a <= altitude))

void	CViewParameters::RenderPlanes(
	short	numPlanes,
	short	*colorList,
	Fixed	*altitudeList)
{
	//	Tables for truncated pyramid edges. (Vertice indices)
	static	short	aClipTable[] = { 0, 0, 1, 2,  4, 4, 5, 6,  0, 1, 2, 3 };
	static	short	bClipTable[] = { 1, 2, 3, 3,  5, 6, 7, 7,  4, 5, 6, 7 };

	Vector	globalC[8];
	Matrix	viewInverse;

	short	i;
	Boolean	doesClip[12];
	Point	clipped[12];
	short	numClipped = 0;
	
	Point	botRight;
	
	botRight = viewPixelDimensions;
	
	InverseTransform(&viewMatrix, &viewInverse);
	viewInverse[3][0] = viewInverse[3][0] PYRAMIDSCALE;
	viewInverse[3][1] = viewInverse[3][1] PYRAMIDSCALE;
	viewInverse[3][2] = viewInverse[3][2] PYRAMIDSCALE;
	VectorMatrix34Product(8, corners, globalC, &viewInverse);

	while(numPlanes--)
	{	Fixed	altitude;
	
		altitude = (*altitudeList++) PYRAMIDSCALE;
		for(i=0;i<12;i++)
		{	doesClip[i] = DIFFSIDES(globalC[aClipTable[i]][1], globalC[bClipTable[i]][1]);
			numClipped++;
		}
		
		if(numClipped >= 3)
		{	if(doesClip[0])
			{	clipped[0].h = 0;
				clipped[0].v = FMulDivNZ(botRight.v,
											globalC[0][1] - altitude,
											globalC[0][1]-globalC[1][1]);
			}
			if(doesClip[1])
			{	clipped[1].v = 0;
				clipped[1].h = FMulDivNZ(botRight.h,
											globalC[0][1] - altitude,
											globalC[0][1]-globalC[2][1]);
			}
			
			if(doesClip[2])
			{	clipped[2].v = botRight.v;
				clipped[2].h = FMulDivNZ(botRight.h,
											globalC[1][1] - altitude,
											globalC[1][1]-globalC[3][1]);
			}
			if(doesClip[3])
			{	clipped[3].h = botRight.h;
				clipped[3].v = FMulDivNZ(botRight.v,
											globalC[2][1] - altitude,
											globalC[2][1]-globalC[3][1]);
			}
		
			if(doesClip[4])
			{	clipped[4].h = 0;
				clipped[4].v = FMulDivNZ(botRight.v,
											globalC[4][1] - altitude,
											globalC[4][1]-globalC[5][1]);
			}
		
			if(doesClip[5])
			{	clipped[5].v = 0;
				clipped[5].h = FMulDivNZ(botRight.h,
											globalC[4][1] - altitude,
											globalC[4][1]-globalC[6][1]);
			}
		
			if(doesClip[6])
			{	clipped[6].v = botRight.v;
				clipped[6].h = FMulDivNZ(botRight.h,
											globalC[5][1] - altitude,
											globalC[5][1]-globalC[7][1]);
			}
			if(doesClip[7])
			{	clipped[7].h = botRight.h;
				clipped[7].v = FMulDivNZ(botRight.v,
											globalC[6][1] - altitude,
											globalC[6][1]-globalC[7][1]);
			}
		
			if(doesClip[8])		{	clipped[8].h =	0;			clipped[8].v = 0;			}
			if(doesClip[9])		{	clipped[9].h =	0;			clipped[9].v = botRight.v;	}
			if(doesClip[10])	{	clipped[10].h = botRight.h;	clipped[10].v = 0;			}
			if(doesClip[11])	{	clipped[11].h = botRight.h;	clipped[11].v = botRight.v;	}
			
			{	static short	drawTable[6][4] ={{	0,	1,	3,	2	},
												{	4,	5,	7,	6	},
												{	3,	10,	7,	11	},
												{	1,	8,	5,	10	},
												{	2,	9,	6,	11	},
												{	0,	8,	4,	9	}};
				
				NewSpritePolygon(*colorList++);
	
				for(i=0;i<6;i++)
				{	short	j;
					short	*tp;
					Point	*p1, *p2;
					
					p1 = 0;
					tp = drawTable[i];
					
					for(j=0;j<4;j++)
					{	short	ind = *tp++;
						if(doesClip[ind])
						{	if(p1)
							{	p2 = clipped+ind;
								AddPolySegment(*p1, *p2);
								p1 = p2;
							}
							else
							{	p1 = clipped+ind;
							}
						}
					}
				}
			}
		}
	}
}

void	CViewParameters::CalculateViewPyramidNormals()
{
	normal[0][0] = -screenScale;
	normal[0][1] = 0;	//	No Y component
	normal[0][2] = ((long)viewPixelCenter.h) << 16;
	NormalizeVector(3, normal[0]);

	normal[1][0] = screenScale;
	normal[1][1] = 0;	//	No Y component
	normal[1][2] = ((long)viewPixelDimensions.h - viewPixelCenter.h) << 16;
	NormalizeVector(3, normal[1]);
	
	normal[2][0] = 0;	//	No X component
	normal[2][1] = -screenScale;
	normal[2][2] = ((long)viewPixelCenter.v) << 16;
	NormalizeVector(3, normal[2]);

	normal[3][0] = 0;	//	No X component
	normal[3][1] = screenScale;
	normal[3][2] = ((long)viewPixelDimensions.v - viewPixelCenter.v) << 16;
	NormalizeVector(3, normal[3]);
}

void	CViewParameters::SetViewRect(
	short	width,
	short	height,
	short	centerX,
	short	centerY)
{
	if(	viewPixelCenter.h != centerX ||
		viewPixelCenter.v != centerY ||
		viewPixelDimensions.h != width ||
		viewPixelDimensions.v != height)
	{
		viewPixelCenter.h = centerX;
		viewPixelCenter.v = centerY;
		viewPixelDimensions.h = width;
		viewPixelDimensions.v = height;

		viewWidth = FMulDivNZ(FIX3(220), viewPixelDimensions.h, 640);
		
		Recalculate();
	}
}

void	CViewParameters::Recalculate()
{
	screenScale = FMulDivNZ(	((long)viewPixelDimensions.h)<<16,
									viewDistance,
									viewWidth);
	
	xOffset = ((long)viewPixelCenter.h) << 16;
	yOffset = ((long)viewPixelCenter.v) << 16;

	fWidth = ((long)viewPixelDimensions.h)<<16;
	fHeight = ((long)viewPixelDimensions.v)<<16;

	CalculateViewPyramidNormals();
}

void	CViewParameters::IViewParameters()
{
	Vector			*vt;
	short			i;
	GrafPtr			curPort;

	lightSeed = 1;

	GetPort(&curPort);
	
	for(i=0;i<MAXLIGHTS;i++)
	{	lightMode[i] = kLightOff;
	}

	OneMatrix(&viewMatrix);

	ambientLight = 32768L;

	SetLight(0, 0, FIX(45), FIX3(900) - ambientLight, kLightGlobalCoordinates);

	hitherBound = FIX3(1000);	//	0.5 m
	yonBound = FIX(500);		//	500 m
	horizonBound = yonBound * 4;

	viewPixelDimensions.h = curPort->portRect.right;
	viewPixelDimensions.v = curPort->portRect.bottom;
	viewPixelCenter.h = viewPixelDimensions.h / 2;
	viewPixelCenter.v = viewPixelDimensions.v / 2;

	viewWidth = FMulDivNZ(FIX3(220), viewPixelDimensions.h, 640);
	viewDistance = FIX3(300);	//	Display distance 30 cm

	Recalculate();
	
	dirtyLook = false;
	inverseDone = false;
}

void	CViewParameters::Dispose()
{
	inherited::Dispose();
}

void	CViewParameters::LookFrom(
	Fixed	x,
	Fixed	y,
	Fixed	z)
{
	fromPoint[0] = x;
	fromPoint[1] = y;
	fromPoint[2] = z;

	dirtyLook = true;
}

void	CViewParameters::LookAt(
	Fixed	x,
	Fixed	y,
	Fixed	z)
{
	atPoint[0] = x;
	atPoint[1] = y;
	atPoint[2] = z;

	dirtyLook = true;
}

void	CViewParameters::LookAtPart(
	CBSPPart	*thePart)
{
	Fixed	*v;
	
	v = thePart->itsTransform[3];
	LookAt(v[0], v[1], v[2]);
}

void	CViewParameters::LookFromPart(
	CBSPPart	*thePart)
{
	Fixed	*v;
	
	v = thePart->itsTransform[3];
	LookFrom(v[0], v[1], v[2]);
}

void	CViewParameters::PointCamera()
{
	if(dirtyLook)
	{
		Matrix	ma,mb,mc;
		Vector	delta;
		Fixed	smallVector[2];
		Fixed	length;
		
		dirtyLook = false;
		inverseDone = false;


		delta[0] = atPoint[0] - fromPoint[0];
		delta[1] = atPoint[1] - fromPoint[1];
		delta[2] = atPoint[2] - fromPoint[2];
	
		OneMatrix(&ma);
		ma[3][0] = -fromPoint[0];
		ma[3][1] = -fromPoint[1];
		ma[3][2] = -fromPoint[2];
		
		OneMatrix(&mb);
		smallVector[0] = delta[0];
		smallVector[1] = delta[2];
		
		length = VectorLength(2,smallVector);
		mb[0][0] = FDiv(smallVector[1],length);
		mb[2][2] = mb[0][0];
		mb[0][2] = FDiv(smallVector[0],length);
		mb[2][0] = - mb[0][2];
		
		CombineTransforms(&ma, &mc, &mb);
		
		OneMatrix(&mb);
		
		smallVector[1] = length;
		smallVector[0] = delta[1];
		length = VectorLength(2,smallVector);
	
		mb[1][1] = FDiv(smallVector[1],length);
		mb[2][2] = mb[1][1];
		mb[1][2] = FDiv(smallVector[0],length);
		mb[2][1] = - mb[1][2];

		CombineTransforms(&mc, &viewMatrix, &mb);		
	}
}

void	CViewParameters::SetLightValues(
	short	n,
	Fixed	dx,
	Fixed	dy,
	Fixed	dz,
	short	mode)
{
	if(n>=0 && n < MAXLIGHTS)
	{	
		if(		lightMode[n] != mode
			||	internalLights[n][0] != dx
			||	internalLights[n][1] != dy
			||	internalLights[n][2] != dz)
		{	lightSeed++;

			if(lightSeed == 0)
				lightSeed = 1;
	
			lightMode[n] = mode;
			internalLights[n][0] = dx;
			internalLights[n][1] = dy;
			internalLights[n][2] = dz;
		}
	}
}

void	CViewParameters::SetLight(
	short	n,
	Fixed	angle1,
	Fixed	angle2,
	Fixed	intensity,
	short	mode)
{
	Fixed	x,y,z;

	if(n>=0 && n < MAXLIGHTS)
	{	x = FMul(FDegCos(angle1), intensity);
		y = FMul(FDegSin(-angle1), intensity);
		z = FMul(FDegCos(angle2), x);
		x = FMul(FDegSin(-angle2), x);
	
		SetLightValues(n, x, y, z, mode);
	}
}

void	CViewParameters::DoLighting()
{
	short	i;
	Vector	*v;
	Boolean	invDone = false;
	Matrix	invViewMatrix;
	
	v = lightSources;
	lightSourceCount = 0;
	
	InverseTransform(&viewMatrix, &invViewMatrix);

	for(i=0;i<MAXLIGHTS;i++)
	{	if(lightMode[i])
		{	lightSourceCount++;
			if(lightMode[i] == kLightGlobalCoordinates)
			{	(*v)[0] = internalLights[i][0];
				(*v)[1] = internalLights[i][1];
				(*v)[2] = internalLights[i][2];
			}
			else
			{	if(!invDone)
				{	invDone = true;
					
					invViewMatrix[0][0] = viewMatrix[0][0];
					invViewMatrix[0][1] = viewMatrix[1][0];
					invViewMatrix[0][2] = viewMatrix[2][0];

					invViewMatrix[1][0] = viewMatrix[0][1];
					invViewMatrix[1][1] = viewMatrix[1][1];
					invViewMatrix[1][2] = viewMatrix[2][1];

					invViewMatrix[2][0] = viewMatrix[0][2];
					invViewMatrix[2][1] = viewMatrix[1][2];
					invViewMatrix[2][2] = viewMatrix[2][2];

				}
				
				VectorMatrix33Product(1, &(internalLights[i]), v, &invViewMatrix);
			}
			
			v++;
		}
	}	
}

void	CViewParameters::SetMatrix(
	Matrix	*theMatrix)
{
	*(MatrixStruct *)viewMatrix = *(MatrixStruct *)theMatrix;
	inverseDone = false;

}


Matrix	*CViewParameters::GetInverseMatrix()
{
	if(!inverseDone)
	{	inverseDone = true;
		InverseTransform(&viewMatrix, &inverseViewMatrix);
	}
	
	return &inverseViewMatrix;
}