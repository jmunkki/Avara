/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CSmartBox.c
    Created: Thursday, November 9, 1995, 12:20
    Modified: Wednesday, January 24, 1996, 5:22
*/

#include "CSmartBox.h"

#define	DIMEPSILON	16

typedef struct
{
	Fixed	baseSize;
	short	scaleStyle;
} bspsResource;

void CSmartBox::ScaleTemplate(
	Fixed			*dimensions,
	Fixed			baseSize)
{
	Vector		*p;
	short		i;
	Fixed		x,y,z;
	ColorRecord	*oldColors;
	Vector		normalAdjust;

	x = dimensions[0];
	y = dimensions[1];
	z = dimensions[2];
	
	if(y == 0)
		y = baseSize;

	header = *(BSPResourceHeader *)*itsBSPResource;

	p = (Vector *)(header.pointOffset + *itsBSPResource);
	for(i=0;i<header.pointCount;i++)
	{	(*p)[0] = FMulDiv((*p)[0], x, baseSize);
		(*p)[1] = FMulDiv((*p)[1], y, baseSize);
		(*p)[2] = FMulDiv((*p)[2], z, baseSize);
		p++;
	}
	
	if(dimensions[0] > DIMEPSILON && dimensions[1] > DIMEPSILON && dimensions[2] > DIMEPSILON)
	{	normalAdjust[0] = FDiv(FIX(1), x);
		normalAdjust[1] = FDiv(FIX(1), y);
		normalAdjust[2] = FDiv(FIX(1), z);
		NormalizeVector(3, normalAdjust);
	
		p = (Vector *)(header.vectorOffset + *itsBSPResource);
		for(i=0;i<header.vectorCount;i++)
		{	x = (*p)[0];
			y = (*p)[1];
			z = (*p)[2];
		
			if((x && (y || z)) || (y && z))
			{	(*p)[0] = FMul(x, normalAdjust[0]);
				(*p)[1] = FMul(y, normalAdjust[1]);
				(*p)[2] = FMul(z, normalAdjust[2]);
				NormalizeVector(3, *p);
			}
			p++;
		}
	}

	FindEnclosure();		
}

void CSmartBox::StretchTemplate(
	Fixed			*dimensions,
	Fixed			baseSize)
{
	Vector		*p;
	short		i;
	Fixed		x,y,z;
	ColorRecord	*oldColors;
	Vector		normalAdjust;
	Fixed		dx,dy,dz;
	Fixed		stretchBound;
	
	stretchBound = baseSize >> 1;
	dx = dimensions[0] - baseSize;
	dy = dimensions[1] - baseSize;
	dz = dimensions[2] - baseSize;
		
	header = *(BSPResourceHeader *)*itsBSPResource;

	p = (Vector *)(header.pointOffset + *itsBSPResource);
	for(i=0;i<header.pointCount;i++)
	{
		x = (*p)[0];
		y = (*p)[1];
		z = (*p)[2];
		
		if(x < -stretchBound)		(*p)[0] -= dx;
		else if(x > stretchBound)	(*p)[0] += dx;
		
		if(y < -stretchBound)		(*p)[1] -= dy;
		else if(y > stretchBound)	(*p)[1] += dy;

		if(z < -stretchBound)		(*p)[2] -= dz;
		else if(z > stretchBound)	(*p)[2] += dz;

		p++;
	}
	
	FindEnclosure();		
}

#define	kMarkerColor		0x00fefefe
#define	kOtherMarkerColor	0x00fe0000

void CSmartBox::ISmartBox(
	short			resId,
	Fixed			*dimensions,
	long			color,
	long			altColor,
	CAbstractActor	*anActor,
	short			aPartCode)
{
	OSErr			iErr;
	Vector			*p;
	short			i;

	itsBSPResource = GetResource(BSPTEMPLATETYPE, resId);

	if(itsBSPResource == NULL)
	{	resId = dimensions[1] ? BOXTEMPLATERESOURCE : PLATETEMPLATERESOURCE;
		itsBSPResource = GetResource(BSPTEMPLATETYPE, resId);
	}

	iErr = HandToHand(&itsBSPResource);
	if(iErr)
	{	itsBSPResource = NULL;
	}
	else
	{	Fixed			x,y,z;
		ColorRecord		*oldColors;
		bspsResource	**config;
		Boolean			stretchFlag;
		Fixed			baseSize;

		config = (bspsResource **)GetResource(BSPSCALETYPE, resId);
		if(config)
		{	stretchFlag = (*config)->scaleStyle;
			baseSize = (*config)->baseSize;
		}
		else
		{	stretchFlag = false;
			baseSize = FIX(1);
		}

		if(stretchFlag)
			ScaleTemplate(dimensions, baseSize);
		else
			StretchTemplate(dimensions, baseSize);

		oldColors = (ColorRecord *)(header.colorOffset + *itsBSPResource);

		for(i=0;i<header.colorCount;i++)
		{	if(oldColors->theColor == kMarkerColor)
				oldColors->theColor = color;
			else if(oldColors->theColor == kOtherMarkerColor)
				oldColors->theColor = altColor;
			
			oldColors++;
		}
		
		*(BSPResourceHeader *)*itsBSPResource = header;
	}
	
	Initialize();
	MoveDone();

	theOwner = anActor;
	partCode = aPartCode;
	
	rSquare[0] = 0;
	rSquare[1] = 0;
	FSquareAccumulate(header.enclosureRadius, rSquare);
}

void CSmartBox::Dispose()
{
	Handle				handCopy;
	BSPResourceHeader	*bp;
	
	handCopy = itsBSPResource;
	bp = (BSPResourceHeader *)*handCopy;
	bp->refCount = 99;	//	Prevent ReleaseResource call!
	
	inherited::Dispose();
	
	DisposeHandle(handCopy);
}

void	CSmartBox::FindEnclosure()
{
	//	Uses algorithm from Graphics Gems I
	
	long		i;
	
	Fixed		xspan, yspan, zspan;
	Fixed		maxspan;
	Fixed		rad, radsq;

	UniquePoint	xmin, xmax, ymin, ymax, zmin, zmax;
	UniquePoint	dia1, dia2, cen;
	UniquePoint	dx, dy, dz;
	UniquePoint	*p;
	
	xmin.x = ymin.y = zmin.z = 0x7fFFffFF;
	xmax.x = ymax.y = zmax.z = -0x7fFFffFF;

	p = (UniquePoint *)(header.pointOffset + *itsBSPResource);
	for(i=0;i<header.pointCount;i++)
	{	if(p->x < xmin.x) xmin = *p;
		if(p->y < ymin.y) ymin = *p;
		if(p->z < zmin.z) zmin = *p;
		if(p->x > xmax.x) xmax = *p;
		if(p->y > ymax.y) ymax = *p;
		if(p->z > zmax.z) zmax = *p;
		
		p++;
	}
	
	header.minBounds.x = xmin.x;
	header.minBounds.y = ymin.y;
	header.minBounds.z = zmin.z;

	header.maxBounds.x = xmax.x;
	header.maxBounds.y = ymax.y;
	header.maxBounds.z = zmax.z;

	dx.x = xmax.x - xmin.x;	dx.y = xmax.y - xmin.y;	dx.z = xmax.z - xmin.z;
	dy.x = ymax.x - ymin.x;	dy.y = ymax.y - ymin.y;	dy.z = ymax.z - ymin.z;
	dz.x = zmax.x - zmin.x;	dz.y = zmax.y - zmin.y;	dz.z = zmax.z - zmin.z;
	
	xspan = VectorLength(3, &dx.x);
	yspan = VectorLength(3, &dy.x);
	zspan = VectorLength(3, &dz.x);

	maxspan = xspan;
	dia1 = xmin;
	dia2 = xmax;
	
	if(yspan > maxspan)
	{	maxspan = yspan;
		dia1 = ymin;
		dia2 = ymax;
	}

	if(zspan > maxspan)
	{	maxspan = zspan;
		dia1 = zmin;
		dia2 = zmax;
	}
	
	cen.x = (dia1.x + dia2.x) / 2;
	cen.y = (dia1.y + dia2.y) / 2;
	cen.z = (dia1.z + dia2.z) / 2;
	
	rad = maxspan / 2;
	
	p = (UniquePoint *)(header.pointOffset + *itsBSPResource);
	for(i=0;i<header.pointCount;i++)
	{	Fixed	newrad;
	
		dx.x = p->x - cen.x;
		dx.y = p->y - cen.y;
		dx.z = p->z - cen.z;
		
		if(FDistanceOverEstimate(dx.x, dx.y, dx.z) > rad)
		{	newrad = VectorLength(3, &dx.x);
			if(newrad > rad)
			{	Fixed	oldtonew;
			
				rad = (rad + newrad) / 2;
				
				oldtonew = newrad - rad;
				
				cen.x = FMulDiv(rad, cen.x, newrad) + FMulDiv(oldtonew, p->x, newrad);
				cen.y = FMulDiv(rad, cen.y, newrad) + FMulDiv(oldtonew, p->y, newrad);
				cen.z = FMulDiv(rad, cen.z, newrad) + FMulDiv(oldtonew, p->z, newrad);
			}
		}
		p++;
	}
	
	header.enclosurePoint = cen;

	p = (UniquePoint *)(header.pointOffset + *itsBSPResource);
	xspan = 0;
	for(i=0;i<header.pointCount;i++)
	{	if(FDistanceOverEstimate(p->x, p->y, p->z) > xspan)
		{	Fixed	temp;
		
			temp = VectorLength(3, &p->x);
			if(temp > xspan)
			{	xspan = temp;
			}
		}
	
		p++;
	}

	header.enclosurePoint.w = xspan;
	if(header.enclosurePoint.w < rad)
	{	rad = header.enclosurePoint.w;
		header.enclosurePoint.x = 0;
		header.enclosurePoint.y = 0;
		header.enclosurePoint.z = 0;
	}
	
	header.enclosureRadius = rad;
}