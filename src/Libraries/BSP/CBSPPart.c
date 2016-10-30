/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CBSPPart.c
    Created: Thursday, June 30, 1994, 10:28
    Modified: Tuesday, July 23, 1996, 4:31
/*/

#include "CBSPPart.h"
#include "BSPPlug.h"
#include "CViewParameters.h"
#include "PolyColor.h"
#include "PAKit.h"
#include "PowerPlugs.h"

#ifndef powerc
/*	The following stacks are used to eliminate recursion.
*/
		short				*bspIndexStack = 0;

static	long				tempLockCount = 0;

static	short				colorLookupSize = 0;

		Vector				**bspPointTemp = 0;
static	long				pointTempMem = 0;

static	BSPLightingPlug		*bspLightingPower = NULL;
static	ClipEdges			*bspClipEdgesPower = NULL;
static	DrawPolygonsPlug	*bspDrawPolygons = NULL;
static	BSPPreProcess		*bspPreProcessPower = NULL;
static	Boolean				bspInitFlag = false;

#define	MINIMUM_TOLERANCE	FIX3(10)

#else
extern	long				*tempLockCountP;
		Vector				**bspPointTemp = 0;
#endif

		ColorRecord			***bspColorLookupTable = 0;

void	CBSPPart::GenerateColorLookupTable()
{
	short		i;
	ColorRecord	*colorP;
	ColorRecord **colorLookupP;

	if(colorReplacements)	colorP = (ColorRecord *) *colorReplacements;
		else				colorP = (ColorRecord *) (header.colorOffset + *itsBSPResource);

	colorLookupP = *bspColorLookupTable;
	
	for(i=0;i<header.colorCount;i++)
	{	*colorLookupP++ = colorP++;
	}
}

/*
**	Unlock locked handles. Note that we are unlocking shared
**	memory structures as well, so if multiple parts are rendered,
**	this routine should only be called after all parts have been
**	completely rendered.
*/
void	CBSPPart::PostRender()
{
	short	*locker;

#ifdef powerc
	if(--(*tempLockCountP) == 0)
#else
	if(--tempLockCount == 0)
#endif
	{	HUnlock((Handle)bspPointTemp);
	}

#ifdef DOLOCKS
	HUnlock(clippingCacheHandle);
	HUnlock(edgeCacheHandle);
	HUnlock((Handle)faceTemp);
#endif

	locker = &((BSPResourceHeader *)*itsBSPResource)->lockCount;
	if(-- (*locker) == 0)
		HUnlock((Handle)itsBSPResource);

}

#ifndef powerc

static	long	seedGoodCount = 0;
static	long	seedUseCount = 0;

void	CBSPPart::TransformLights()
{
	CViewParameters			*vp;
	Matrix					*invFull;

	vp = currentView;
	if(!ignoreDirectionalLights)
	{
		seedUseCount++;

		if(lightSeed != vp->lightSeed)
		{	lightSeed = vp->lightSeed;

			VectorMatrix33Product(vp->lightSourceCount, vp->lightSources,
									objLights, &invGlobTransform);
		}
		else
		{	seedGoodCount++;
		}
	}

	invFull = 1 + (Matrix *)*edgeCacheHandle;
	localViewOrigin[0] = (*invFull)[3][0];
	localViewOrigin[1] = (*invFull)[3][1];
	localViewOrigin[2] = (*invFull)[3][2];
}

/*
**	Do lighting in object space.
*/
void	CBSPPart::DoLighting()
{
	CViewParameters			*vp = currentView;
	Vector					*pointArray;
	Vector					*vectorArray;
	long					i;
	short					j;
	NormalRecord			*norms;
	FaceVisibility			*faces;
	ColorRecord				**colorPointers;
	Fixed					ambient;
	short					lightCount;
	
	if(ignoreDirectionalLights)
	{	lightCount = 0;
	}
	else
	{	lightCount = vp->lightSourceCount;
	}


	if(privateAmbient < 0)
	{	ambient = vp->ambientLight + extraAmbient;
	}
	else
	{	ambient = privateAmbient+extraAmbient;
	}

	norms = (NormalRecord *)(header.normalOffset + *itsBSPResource);
	vectorArray = (Vector *)(header.vectorOffset + *itsBSPResource);
	pointArray = (Vector *)(header.pointOffset + *itsBSPResource);

	colorPointers = *bspColorLookupTable;

	faces = *faceTemp;

	i = header.normalCount;
	while(--i >= 0)
	{				Fixed	light;
					Fixed	dot;
		register	Fixed	x,y,z;
					Fixed	*lv;
					Fixed	*surfNorm;
					Fixed	*surfPoint;
		
		//	Access surface normal and one point on surface.
		surfNorm = vectorArray[norms->normalIndex];
		surfPoint = pointArray[norms->basePointIndex];

		//	Determine if surface faces the user.
		//	This information is used for backface culling and
		//	walking the BSP tree in the right order.
		
		x = *surfNorm++;
		y = *surfNorm++;
		z = *surfNorm;
		dot = 	FMul(x, *surfPoint++ - localViewOrigin[0]);
		dot +=	FMul(y, *surfPoint++ - localViewOrigin[1]);
		dot +=	FMul(z, *surfPoint - localViewOrigin[2]);

		if(dot >= 0)
		{	faces->visibility = frontVisible;
		}
		else
		{	faces->visibility = backVisible;
		}

		//	If this face will be drawn, calculate lights for it:
		if(faces->visibility & norms->visibilityFlags)
		{
			//	Flip normal so that it points to the user:
			if(dot < 0)
			{	x = -x;
				y = -y;
				z = -z;
			}

			light = ambient;
			lv = objLights[0];
			
			j = lightCount;
			while(--j >= 0)
			{	
				dot =	FMul(*lv++, x);
				dot +=	FMul(*lv++, y);
				dot +=	FMul(*lv++, z);
	
				if(dot > 0) light += dot;
				
				lv++;
			}
			
			j = FIXEDTOCOLORCACHE(light);

			if(j < 0)						j = 0;
			else if(j >= COLORCACHESIZE)	j = COLORCACHESIZE-1;
			
			faces->colorIndex = colorPointers[norms->colorIndex]->colorCache[j];
		}
		norms++;
		faces++;
	}
}
#ifdef	DEBUGSTATS
static	long	debugDrawCount = 0;
#endif

/*
**	DrawPolygons used to be a recursive binary space partition tree walk.
**	I removed the recursion and use a stack instead. A negative value (i)
**	on top of the stack means that polygon -i is next in the tree and
**	should push it's "commands" in the stack. A positive value means
**	that you draw a polygon and drop the index from the stack.
*/
void	CBSPPart::DrawPolygons()
{
	PolyRecord		*thePoly;
	short			*stackP;
	short			*stackTop;
	short			i;
	short			polyVisibility;
	EdgeIndex		*e;
	PolyEdgePtr		*quickEdge;

#if 0
	if(bspDrawPolygons)
	{	DrawPolygonsParam	param;
	
		param.polys = GetPolyWorld();
		param.faceTemp = faceTemp;
		param.polyTable = polyTable;
		param.clippingCacheHandle = clippingCacheHandle;
		param.bspPointTemp = bspPointTemp;
		param.edgeCacheHandle = edgeCacheHandle;
		param.edgeTable = edgeTable;
		param.bspIndexStack = bspIndexStack;
		bspDrawPolygons(&param);
#ifdef	DEBUGSTATS
		debugDrawCount++;
#endif
	}
	else
#endif
	{
		quickEdge = (PolyEdgePtr *)*bspPointTemp;
		clippingCacheTable = *clippingCacheHandle;
		edgeCacheTable = *edgeCacheHandle;
		faceTable = *faceTemp;
	
		thePoly = polyTable;
		stackTop = INDEXSTACKDEPTH + bspIndexStack;
		stackP = stackTop;
	
		//	The root polygon has to be handled outside the loop,
		//	because its index is 0.
		
		polyVisibility = faceTable[thePoly->normalIndex].visibility;
		
		if(polyVisibility == frontVisible)
		{
			if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
			if(thePoly->visibility & polyVisibility) *--stackP = 0;
			if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
		}
		else
		{
			if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
			if(thePoly->visibility & polyVisibility) *--stackP = 0;
			if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
		}
	
		while(stackP != stackTop)
		{	i = *stackP++;
		
			if(i<0)	//	A negative index means that we haven't looked at this face yet.
			{
				thePoly = polyTable - i;
	
				polyVisibility = faceTable[thePoly->normalIndex].visibility;
				if(polyVisibility == frontVisible)
				{
					if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
					if(thePoly->visibility & polyVisibility)	*--stackP = -i;
					if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
				}
				else
				{
					if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
					if(thePoly->visibility & polyVisibility) *--stackP = -i;
					if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
				}
			}
			else	//	Positive index means that we should draw this face.
			{	short			frontCount = 0;
				short			backCount = 0;
				Fixed2DPoint	frontList[MAXCROSSPOINTS];
				Fixed2DPoint	backList[MAXCROSSPOINTS];
				Fixed2DPoint	*p;
				Point			pa,pb;
				short			j;
	
				thePoly = polyTable + i;
				e = &edgeTable[thePoly->firstEdge];
	
				NewSpritePolygon(faceTable[thePoly->normalIndex].colorIndex);
	
				for(i=thePoly->edgeCount;i;i--)
				{	short		clipFlags;
					short		edgeInd;
					
					edgeInd = *e++;
					clipFlags = clippingCacheTable[edgeInd];
		
					if(clipFlags)
					{	EdgeCacheRecord		*screenEdge;
						
						screenEdge = &edgeCacheTable[edgeInd];
	
						if(clipFlags & touchFrontClip)
						{	ClipListHandler(&frontCount, frontList,
									screenEdge->ax.f, screenEdge->ay.f);
						}
						
						if(clipFlags & touchBackClip)
						{	ClipListHandler(&backCount, backList,
									screenEdge->bx.f, screenEdge->by.f);
						}
	
						if(clipFlags < 0)
						{	PolyEdgePtr	previous;
						
							previous = quickEdge[edgeInd];
							if(previous) ReplicatePolySegment(previous);
						}
						else
						{	clippingCacheTable[edgeInd] |= edgeExitsFlag;
							quickEdge[edgeInd] =
								AddFPolySegment(screenEdge->ax.f, screenEdge->ay.f,
											screenEdge->bx.f, screenEdge->by.f);
						}
					}
				}
	
				p = frontList;
				for(j=1;j<frontCount;j+=2)
				{	AddFPolySegment(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
					p+=2;
				}
				p = backList;
				for(j=1;j<backCount;j+=2)
				{	AddFPolySegment(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
					p+=2;
				}
			}
		}
	}
}

/*
**	This is a special version of DrawPolygons that assigns a different color
**	number for each polygon. It also draws all polygons regardless of how they
**	are facing. It is not used for normal rendering.
**
**	I hate copying this large amount of code, but I don't want to add this
**	functionality to the much more timing critical DrawPolygons method.
*/
void	CBSPPart::DrawNumberedPolygons()
{
	PolyRecord		*thePoly;
	short			*stackP;
	short			*stackTop;
	short			i;
	short			polyVisibility;
	EdgeIndex		*e;

	clippingCacheTable = *clippingCacheHandle;
	edgeCacheTable = *edgeCacheHandle;
	faceTable = *faceTemp;

	thePoly = polyTable;
	stackTop = INDEXSTACKDEPTH + bspIndexStack;
	stackP = stackTop;

	//	The root polygon has to be handled outside the loop,
	//	because its index is 0.
	
	polyVisibility = faceTable[thePoly->normalIndex].visibility;
	
	if(polyVisibility == frontVisible)
	{
		if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
		*--stackP = 0;
		if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
	}
	else
	{
		if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
		*--stackP = 0;
		if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
	}

	while(stackP != stackTop)
	{	i = *stackP++;
	
		if(i<0)	//	A negative index means that we haven't looked at this face yet.
		{
			thePoly = polyTable - i;

			polyVisibility = faceTable[thePoly->normalIndex].visibility;
			if(polyVisibility == frontVisible)
			{
				if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
				*--stackP = -i;
				if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
			}
			else
			{
				if(thePoly->frontPoly > 0)	*--stackP = -thePoly->frontPoly;
				*--stackP = -i;
				if(thePoly->backPoly > 0)	*--stackP = -thePoly->backPoly;
			}
		}
		else	//	Positive index means that we should draw this face.
		{	short			frontCount = 0;
			short			backCount = 0;
			Fixed2DPoint	frontList[MAXCROSSPOINTS];
			Fixed2DPoint	backList[MAXCROSSPOINTS];
			Fixed2DPoint	*p;
			Point			pa,pb;
			short			j;

			thePoly = polyTable + i;
			e = &edgeTable[thePoly->firstEdge];

			NewSpritePolygon(i);	//	Use polygon indices for colors.
		
			for(i=thePoly->edgeCount;i;i--)
			{	short		clipFlags;
				
				clipFlags = clippingCacheTable[*e];
	
				if(clipFlags)
				{	EdgeCacheRecord		*screenEdge;
					
					screenEdge = &edgeCacheTable[*e];
	
					if(clipFlags & touchFrontClip)
					{	ClipListHandler(&frontCount, frontList,
								screenEdge->ax.f, screenEdge->ay.f);
					}
					
					if(clipFlags & touchBackClip)
					{	ClipListHandler(&backCount, backList,
								screenEdge->bx.f, screenEdge->by.f);
					}

					AddFPolySegment(screenEdge->ax.f, screenEdge->ay.f,
									screenEdge->bx.f, screenEdge->by.f);
				}
				
				e++;
			}

			p = frontList;
			for(j=1;j<frontCount;j+=2)
			{	AddFPolySegment(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
				p+=2;
			}
			p = backList;
			for(j=1;j<backCount;j+=2)
			{	AddFPolySegment(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
				p+=2;
			}
		}
	}
}

void	CBSPPart::MarkNode()
{
	short			polyIndex = 0;
	PolyRecord		*thePoly;
	FaceVisibility	*thisFace;
	short			i;
	EdgeIndex		*e;
	short			*stat;

	short			*indexStackP;

	stat = clippingCacheTable;
	for(i=0;i<header.uniqueEdgeCount;i++)
	{	*stat++ = false;
	}
	
	indexStackP = bspIndexStack;
	*indexStackP++ = -1;
	do
	{
		do
		{	thePoly = polyIndex + polyTable;
			thisFace = thePoly->normalIndex + faceTable;
		
			if(thePoly->visibility & thisFace->visibility)
			{	e = &edgeTable[thePoly->firstEdge];
			
				for(i=thePoly->edgeCount;i;i--)
				{	clippingCacheTable[*e++] = true;
				}
			}
		
			if(thePoly->backPoly > 0)
			{	*indexStackP++ = thePoly->backPoly;
			}
	
			polyIndex = thePoly->frontPoly;
	
		} while(polyIndex>0);
		polyIndex = * (--indexStackP);

	} while(polyIndex > 0);
}

void	CBSPPart::MarkPoints()
{
	short		i;
	Vector		*v;
	EdgeRecord	*uniEdge;
	Vector		*pointArray;
	Fixed		used = FIX(1);
	short		*stat;
	
	pointArray = (Vector *)(header.pointOffset + *itsBSPResource);
	
	v = pointArray;
	
	for(i=0;i<header.pointCount;i++)
	{	(*v++)[3] = 0;	//	Mark as not being used.
	}
	
	uniEdge = uniqueEdgeTable;
	stat = clippingCacheTable;

	for(i=0;i<header.uniqueEdgeCount;i++)
	{	if(*(stat++))
		{	pointArray[uniEdge->a][3] = used;
			pointArray[uniEdge->b][3] = used;
		}
		uniEdge++;
	}
}

#ifdef TESTING_OVERFLOWS
/*
**	Here's a special version for the perspective transformation.
**	It alleviates the need for viewing pyramid clipping by detecting
**	overflow. In cases of overflow it returns 0x3FFF FFFF when
**	a * b >= 0 and -0x3FFF FFFF when a * b < 0.
*/
Fixed	zFMulDivVNZ(
Fixed	a,
Fixed	b,
Fixed	c);
Fixed	zFMulDivVNZ(
Fixed	a,
Fixed	b,
Fixed	c)
{
asm	{
	move.l	a,D0
	move.l	c,D2
	muls.l	b,D1:D0
	bvc.s	@mulOk
	asr.l	#1,D0
	muls.l	b,D1:D0
	asr.l	#1,D2
@mulOk
	divs.l	D2,D1:D0
	bvc.s	@end		//	Test for overflow
	moveq.l	#-1,D0
	lsr.l	#2,D0		//	D0 = 0x3FFF FFFF
	tst.l	D1
	bpl.s	@infinity
	neg.l	D0
@end
@infinity

	}
}
#endif
void	CBSPPart::ClipEdges()
{
	EdgeCacheRecord		*clippedLine;
	short				i;
	short				*stat;
	EdgeRecord			*uniEdge;
	Vector				frontClipper, backClipper;	//	Used to contain clipped coordinates
	Fixed				xOffCopy;
	Fixed				yOffCopy;
	Fixed				negScaleCopy;

	xOffCopy = currentView->xOffset;
	yOffCopy = currentView->yOffset;
	negScaleCopy = -currentView->screenScale;

	minZ = yon;
	maxZ = hither;
	minX = 0x7FffFFff;
	maxX = 0x80000000;
	minY = minX;
	maxY = maxX;

	stat = clippingCacheTable;
	clippedLine = edgeCacheTable;
	uniEdge = uniqueEdgeTable;

	for(i=0;i<header.uniqueEdgeCount;i++)
	{	if(*stat)
		{	Vector	*va, *vb;
			Fixed	az, bz;

			va = &pointTable[uniEdge->a];
			vb = &pointTable[uniEdge->b];
			
			//	Sort endpoints:
			if((*va)[2] > (*vb)[2])
			{	Vector	*vt;
				vt = va;
				va = vb;
				vb = vt;
			}
			
			az = (*va)[2];
			bz = (*vb)[2];
			
			if(az >= yon)
					*stat = noLineClip;	//	Line is too far to be visible
			else
			if(bz <= hither)
					*stat = noLineClip;	//	Line is too close or behind
			else
			{	Fixed	clipDist;
				Fixed	deltaZ;
				
				*stat = lineVisibleClip;
				
				if(az <= minZ)
				{
					clipDist = az - hither;
					if(clipDist <= 0)
					{	if(clipDist)
						{	deltaZ = az - bz;
							
							frontClipper[0] = (*va)[0] + FMulDivNZ((*vb)[0] - (*va)[0], clipDist, deltaZ);
							frontClipper[1] = (*va)[1] + FMulDivNZ((*vb)[1] - (*va)[1], clipDist, deltaZ);
							va = &frontClipper;

							az = hither;
						}
						
						*stat |= touchFrontClip;
					}
					
					minZ = az;
				}
				
				if(bz >= maxZ)
				{	
					clipDist = bz - yon;
					if(clipDist >= 0)
					{	if(clipDist)
						{	deltaZ = bz - az;
							
							backClipper[0] = (*vb)[0] + FMulDivNZ((*va)[0] - (*vb)[0], clipDist, deltaZ);
							backClipper[1] = (*vb)[1] + FMulDivNZ((*va)[1] - (*vb)[1], clipDist, deltaZ);
							vb = &backClipper;

							bz = yon;
						}

						*stat |= touchBackClip;
					}
					
					maxZ = bz;
				}

				/*
				**	FMulDivV is a special version of the FMulDiv call. It detects overflow
				**	before the division and returns a result depending on the sign of the
				**	product. Thus, even without full pyramid clipping, the values do not
				**	"bounce randomly" across the screen smudging everything under them.
				**
				**	Addendum:	the NZ means "Never Zero" and can be used in conditions when
				**				a divide by zero is guaranteed never to happen.
				*/
				clippedLine->ax.f = FMulDivVNZ(negScaleCopy, (*va)[0], az) + xOffCopy;
				clippedLine->ay.f = FMulDivVNZ(negScaleCopy, (*va)[1], az) + yOffCopy;
				
				clippedLine->bx.f = FMulDivVNZ(negScaleCopy, (*vb)[0], bz) + xOffCopy;
				clippedLine->by.f = FMulDivVNZ(negScaleCopy, (*vb)[1], bz) + yOffCopy;
				
				if(clippedLine->ax.f < clippedLine->bx.f)
				{	if(clippedLine->ax.f < minX) minX = clippedLine->ax.f;
					if(clippedLine->bx.f > maxX) maxX = clippedLine->bx.f;
				}
				else
				{	if(clippedLine->bx.f < minX) minX = clippedLine->bx.f;
					if(clippedLine->ax.f > maxX) maxX = clippedLine->ax.f;
				}

				if(clippedLine->ay.f < clippedLine->by.f)
				{	if(clippedLine->ay.f < minY) minY = clippedLine->ay.f;
					if(clippedLine->by.f > maxY) maxY = clippedLine->by.f;
				}
				else
				{	if(clippedLine->by.f < minY) minY = clippedLine->by.f;
					if(clippedLine->ay.f > maxY) maxY = clippedLine->ay.f;
				}
			}
		}
		stat++;
		clippedLine++;
		uniEdge++;
	}
}

Boolean	CBSPPart::InViewPyramid()
{
	CViewParameters	*vp;
	short			i;
	Vector			*norms;
	Fixed			radius;
	Fixed			distance;
	Fixed			x,y;
	Fixed			z;
	
	vp = currentView;
	
	if(hither >= yon)
		return false;

	z = vp->viewMatrix[3][2];
	z += FMul(sphereGlobCenter[0], vp->viewMatrix[0][2]);
	z += FMul(sphereGlobCenter[1], vp->viewMatrix[1][2]);
	z += FMul(sphereGlobCenter[2], vp->viewMatrix[2][2]);
	minZ = z - header.enclosureRadius;
	maxZ = z + header.enclosureRadius;

	if(	minZ > yon || maxZ < hither)
	{	return false;
	}

	VectorMatrix34Product(1, &sphereGlobCenter, &sphereCenter, &vp->viewMatrix);
	z = sphereCenter[2];
	x = sphereCenter[0];
	radius = -header.enclosureRadius;

	distance  = FMul(x, vp->normal[0][0]);
	distance += FMul(z, vp->normal[0][2]);
	if(distance < radius)	return false;

	distance  = FMul(x, vp->normal[1][0]);
	distance += FMul(z, vp->normal[1][2]);
	if(distance < radius)	return false;

	y = sphereCenter[1];
	distance =	FMul(y, vp->normal[2][1]);
	distance += FMul(z, vp->normal[2][2]);
	if(distance < radius)	return false;

	distance =	FMul(y, vp->normal[3][1]);
	distance += FMul(z, vp->normal[3][2]);
	if(distance < radius)	return false;
	
	return true;
}

/*
**	Lock handles and make pointers to data valid.
*/
void	CBSPPart::PreRender()
{
	short	*locker;

	locker = &((BSPResourceHeader *)*itsBSPResource)->lockCount;
	if((*locker)++ == 0)
		HLock(itsBSPResource);

	if(tempLockCount++ == 0)
	{	HLock((Handle)bspPointTemp);
	}

	pointTable = *bspPointTemp;
	clippingCacheTable = *clippingCacheHandle;
	edgeCacheTable = *edgeCacheHandle;
	faceTable = *faceTemp;

	edgeTable = (EdgeIndex *)(header.edgeOffset + *itsBSPResource);
	uniqueEdgeTable = (EdgeRecord *)(header.uniqueEdgeOffset + *itsBSPResource);
	polyTable = (PolyRecord *)(header.polyOffset + *itsBSPResource);
}

#ifdef	DEBUGSTATS
static	long	debugPrepCount = 0;
#endif

/*
**	See if the part is in the viewing pyramid and do calculations
**	in preparation to shading.
*/
Boolean	CBSPPart::PrepareForRenderOne(
	CViewParameters	*vp)
{
	Boolean	inPyramid = !isTransparent;
	
	if(inPyramid)
	{	currentView = vp;

		if(!usesPrivateHither)	hither = vp->hitherBound;
		if(!usesPrivateYon)		yon = vp->yonBound;
	
		inPyramid = InViewPyramid();
		
		if(inPyramid)
		{	Matrix	*fullAndInvFullTransforms;
		
			fullAndInvFullTransforms = (Matrix *)*edgeCacheHandle;

			CombineTransforms(		&itsTransform,
									fullAndInvFullTransforms,
									&vp->viewMatrix);

			InverseTransform(fullAndInvFullTransforms, fullAndInvFullTransforms+1);

			if(!invGlobDone)
			{	InverseTransform(&itsTransform, &invGlobTransform);
				invGlobDone = true;
			}

			PreRender();
			TransformLights();

		}
	}
	
	return inPyramid;
}

/*
**	Do calculations in preparation to drawing, then check to see if it is on screen.
*/
Boolean	CBSPPart::PrepareForRenderTwo()
{
	Boolean	onScreen;
	Vector	*pointArray;

	GenerateColorLookupTable();

#if 0
	if(bspPreProcessPower)
	{	bspPreProcessPower(&(this->itsBSPResource));
#ifdef	DEBUGSTATS
		debugPrepCount++;
#endif
	}
	else
#endif
	{
		DoLighting();
		MarkNode();
		MarkPoints();

		pointTable = *bspPointTemp;
		pointArray = (Vector *)(header.pointOffset + *itsBSPResource);
		FlaggedVectorMatrix34Product(header.pointCount, pointArray, pointTable, (Matrix *)*edgeCacheHandle);

		ClipEdges();
	}

	onScreen = (maxX > 0 && maxY > 0 &&
				 minX < currentView->fWidth && minY < currentView->fHeight);

	if(!onScreen)
		PostRender();

	return onScreen;
}

/*
**	Normally you would create a CBSPWorld and attach the
**	part to that world. However, if you only have a single
**	CBSPPart, you can call Render and you don't need a
**	CBSPWorld. Even then it is recommended that you use a
**	CBSPWorld, since it really doesn't add any significant
**	overhead.
*/
void	CBSPPart::Render(
	CViewParameters	*vp)
{
	vp->DoLighting();

	if(PrepareForRenderOne(vp) && PrepareForRenderTwo())
	{	DrawPolygons();
		PostRender();
	}
}

/*
**	Reset the part to the origin and to its natural
**	orientation.
*/
void	CBSPPart::Reset()
{
	lightSeed = 0;
	OneMatrix(&itsTransform);
}

//	invalidates data & calcs sphereGlobCenter
void	CBSPPart::MoveDone()
{
	VectorMatrix34Product(1, (Vector *)&header.enclosurePoint, &sphereGlobCenter, &itsTransform);
	invGlobDone = false;
	lightSeed = 0;
}

#ifdef HAS_TRANSLATE_METHODs
/*
**	Move by xt, yt, zt
*/
void	CBSPPart::Translate(
	Fixed	xt,
	Fixed	yt,
	Fixed	zt)
{
	itsTransform[3][0] += xt;
	itsTransform[3][1] += yt;
	itsTransform[3][2] += zt;
}
#endif

void	CBSPPart::RotateX(
	Fixed	angle)
{
	angle = FDegToOne(angle);
	MRotateX(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateY(
	Fixed	angle)
{
	angle = FDegToOne(angle);
	MRotateY(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateZ(
	Fixed	angle)
{
	angle = FDegToOne(angle);
	MRotateZ(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateRadX(
	Fixed	angle)
{
	angle = FRadToOne(angle);
	MRotateX(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateRadY(
	Fixed	angle)
{
	angle = FRadToOne(angle);
	MRotateY(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateRadZ(
	Fixed	angle)
{
	angle = FRadToOne(angle);
	MRotateZ(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateOneX(
	Fixed	angle)
{
	MRotateX(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::RotateOneY(
	Fixed	angle)
{
	MRotateY(FOneSin(angle), FOneCos(angle), &itsTransform);
}
void	CBSPPart::RotateOneZ(
	Fixed	angle)
{
	MRotateZ(FOneSin(angle), FOneCos(angle), &itsTransform);
}

void	CBSPPart::CopyTransform(
	Matrix	*m)
{
	invGlobDone = false;
	*(MatrixStruct *)&itsTransform = *(MatrixStruct *)m;
}

void	CBSPPart::ApplyMatrix(
	Matrix	*m)
{
	Matrix	fullMatrix;
	
	CombineTransforms(&itsTransform, &fullMatrix, m);

	*((MatrixStruct *)&itsTransform) = *(MatrixStruct *)&fullMatrix;
}
void	CBSPPart::PrependMatrix(
	Matrix	*m)
{
	Matrix	fullMatrix;
	
	CombineTransforms(m, &fullMatrix, &itsTransform);

	*((MatrixStruct *)&itsTransform) = *(MatrixStruct *)&fullMatrix;
}

Matrix *CBSPPart::GetInverseTransform()
{
	if(!invGlobDone)
	{	invGlobDone = true;
		InverseTransform(&itsTransform, &invGlobTransform);
	}

	return &invGlobTransform;
}

void	CBSPPart::CacheOneColor(
	ColorRecord		*aColor)
{
	long	maxColorLevel = 255;
	long	r,g,b;
	long	theColor;
	long	overR = 0;
	long	overG = 0;
	long	overB = 0;
	short	i;

	theColor = aColor->theColor;

	r = maxColorLevel & (theColor >> 16);
	g = maxColorLevel & (theColor >> 8);
	b = maxColorLevel & theColor;
	
	for(i=0;i<COLORCACHESIZE;i++)
	{	long	newColor;
		long	newR, newG, newB;
		
		newR = ((r * i) / COLORLEVELONE);
		newG = ((g * i) / COLORLEVELONE);
		newB = ((b * i) / COLORLEVELONE);

		if(newR > maxColorLevel)	overR = (newR - maxColorLevel)>>1;
		if(newG > maxColorLevel)	overG = (newG - maxColorLevel)>>1;
		if(newB > maxColorLevel)	overB = (newB - maxColorLevel)>>1;
		
		newR += overG+overB;	if(newR > maxColorLevel) newR = maxColorLevel;
		newG += overR+overB;	if(newG > maxColorLevel) newG = maxColorLevel;
		newB += overR+overG;	if(newB > maxColorLevel) newB = maxColorLevel;
		
		newColor = newB + (newG << 8) + (newR << 16);
		aColor->colorCache[i] = FindPolyColor(newColor);
	}
}

void	CBSPPart::CacheColors()
{
	short			i;
	
	ColorRecord		*colors;
		
	colors = (ColorRecord *)(header.colorOffset + *itsBSPResource);
	for(i=0;i<header.colorCount;i++)
	{	
		CacheOneColor(colors);
		colors++;
	}
}

void	CBSPPart::ReplaceColor(
	long	origColor,
	long	newColor)
{
	short			i;
	ColorRecord		*oldColors;
	ColorRecord		*newColors;

	//	Make a copy of default colors.
	if(!colorReplacements)
	{	long	colorsSize;

		//	First look to see if we can find the original color...
		oldColors = (ColorRecord *)(header.colorOffset + *itsBSPResource);

		for(i=0;i<header.colorCount;i++)
		{	if(oldColors->theColor == origColor)
				break;
			oldColors++;
		}
		
		if(i == header.colorCount)	//	Not found?
			return;
	
		colorsSize = header.colorCount * sizeof(ColorRecord);
		colorReplacements = NewHandle(colorsSize);
		if(!colorReplacements)
			return;
		
		BlockMoveData(header.colorOffset + *itsBSPResource, *colorReplacements, colorsSize);
	}
	
	//	Look for origColor in original table and replace it with the new one in the
	//	new color table.
	HLock(colorReplacements);
	HLock(itsBSPResource);
	newColors = (ColorRecord *)*colorReplacements;
	oldColors = (ColorRecord *)(header.colorOffset + *itsBSPResource);

	for(i=0;i<header.colorCount;i++)
	{	if(oldColors->theColor == origColor)
		{	newColors->theColor = newColor;
			CacheOneColor(newColors);
		}
		oldColors++;
		newColors++;
	}
	HUnlock(colorReplacements);
	HUnlock(itsBSPResource);
}

void	CBSPPart::BuildBoundingVolumes()
{
	short			i;
	Fixed			span;
	Fixed			radius = 0;
	Fixed			bestRadius = 0;
	Fixed			*dest;
#ifdef OLDCODE
	UniquePoint		pts[2];
	short			jTable[] = { 0, 7, 1, 6, 2, 5, 4, 3 };
/*
**	Binary			000	-> 111
**					001	-> 110
**					010	-> 101
**					100	-> 011
**	The points are reordered so that opposite corners are
**	tested in the overlap and ordering tests.
*/

	pts[0] = header.minBounds;
	pts[1] = header.maxBounds;

	for(i=0;i<8;i++)
	{	dest = boxCorners[jTable[i]];
		dest[0] = pts[i>>2].x;
		dest[1] = pts[(i>>1) & 1].y;
		dest[2] = pts[i & 1].z;
		dest[3] = FIX(1);
	}
#endif


#if 1
	/*	Since version 0.6 of BSPSPlitter, bigRadius is
	**	exactly calculated and placed in enclosurePoint.w.
	*/

	bigRadius = header.enclosurePoint.w;
	header.enclosurePoint.w = FIX(1);
#else
	/*	This fixes BSPT resources previous to 0.6
	*/

	//	Bounding box into an origin-centered sphere
	bestRadius = VectorLength(3, &pts[0].x);
	radius = VectorLength(3, &pts[1].x);
	if(radius = bestRadius) bestRadius = radius;

	//	Bounding sphere into an origin-centered sphere
	radius = header.enclosureRadius + VectorLength(3, &header.enclosurePoint.x);
	if(radius < bestRadius) bestRadius = radius;
	bigRadius = bestRadius;

	header.enclosurePoint.w = FIX(1);
	((BSPResourceHeader *)*itsBSPResource)->enclosurePoint.w = bigRadius;
#endif
	tolerance	= header.maxBounds.x - header.minBounds.x;
	span		= header.maxBounds.y - header.minBounds.y;

	if(span > MINIMUM_TOLERANCE && span < tolerance)
		tolerance = span;

	span	= header.maxBounds.z - header.minBounds.z;
	if(span > MINIMUM_TOLERANCE && span < tolerance)
		tolerance = span;
	
	tolerance >>= 4;
	if(tolerance < MINIMUM_TOLERANCE)	tolerance = MINIMUM_TOLERANCE;
}

void	CBSPPart::RepairColorTableSize()
{
	long	sizeNeeded;
	long	sizeCurrently;
	long	apparentRecordSize;
	long	sizeDelta;
	
	sizeCurrently = header.pointOffset - header.colorOffset;
	sizeNeeded = sizeof(ColorRecord) * header.colorCount;
	sizeDelta = sizeNeeded - sizeCurrently;
	
	if(sizeDelta)
	{	Ptr			wildPointer;
		ColorRecord	*dest;
		short		i;
		long		trailerSize;
		long		resSize;
		
		resSize = GetHandleSize(itsBSPResource);
		apparentRecordSize = sizeCurrently / header.colorCount;
		trailerSize = resSize - header.pointOffset;

		HUnlock(itsBSPResource);

		if(sizeDelta > 0)
		{	SetHandleSize(itsBSPResource, resSize + sizeDelta);
			if(MemError())
			{	SysBeep(10);
				ExitToShell();
			}
			BlockMoveData(	*itsBSPResource+header.pointOffset,
						*itsBSPResource+header.pointOffset + sizeDelta,
						trailerSize);
		}
		wildPointer = header.colorOffset + *itsBSPResource;
		dest = (ColorRecord *) wildPointer;
		
		for(i=0;i<header.colorCount;i++)
		{	dest->theColor = ((ColorRecord *)wildPointer)->theColor;
			wildPointer += apparentRecordSize;
			dest++;
		}
		
		if(sizeDelta < 0)
		{	BlockMoveData(	*itsBSPResource+header.pointOffset,
						*itsBSPResource+header.pointOffset + sizeDelta,
						trailerSize);
			SetHandleSize(itsBSPResource, resSize + sizeDelta);
			if(MemError())
			{	SysBeep(10);
				ExitToShell();
			}
		}
		
		header.pointOffset += sizeDelta;
		header.vectorOffset += sizeDelta;
		header.uniqueEdgeOffset +=sizeDelta;
		*(BSPResourceHeader *)(*itsBSPResource) = header;
		ChangedResource(itsBSPResource);
		WriteResource(itsBSPResource);
	}
}

void	CBSPPart::IBSPPart(
	short	aResId)
{
	itsBSPResource = GetResource(BSPRESTYPE, aResId);
	Initialize();
}

void	CBSPPart::Initialize()
{
	long		pointTempReq;
	long		edgeCacheMem;
	long		clippingCacheMem;
	
	lightSeed = 0;
	nextTemp = NULL;
	colorReplacements = NULL;	//	Use default colors.
	isTransparent = false;
	ignoreDirectionalLights = false;
	
	usesPrivateHither = false;
	usesPrivateYon = false;

	hither = FIX3(500);	//	50 cm	I set these variables just in case some bozo
	yon = FIX(500);	//	500 m	sets the flags above and forgets to set the values.	
	userFlags = 0;
	
	if(itsBSPResource)
	{	BSPResourceHeader	*bp;
	
		header = *(BSPResourceHeader *)*itsBSPResource;

		if(header.refCount == 0)
		{	short	*locker;

			locker = &((BSPResourceHeader *)*itsBSPResource)->lockCount;
			(*locker) = 0;
			RepairColorTableSize();
		}

		HLock(itsBSPResource);
		bp = (BSPResourceHeader *)*itsBSPResource;
		header = *bp;
		extraAmbient = 0;
		privateAmbient = -1;

		faceTemp = (FaceVisibility **) NewHandle(header.normalCount * sizeof(FaceVisibility));

		if(bp->refCount == 0)
			CacheColors();
		
		bp->refCount++;
		
		pointTempReq = header.pointCount * sizeof(Vector);
		edgeCacheMem = header.uniqueEdgeCount * sizeof(Fixed);

		if(edgeCacheMem > pointTempReq)
			pointTempMem = edgeCacheMem;

		if(pointTempReq > pointTempMem)
		{
			pointTempMem = pointTempReq;

			if(bspPointTemp)
			{	SetHandleSize((Handle)bspPointTemp, pointTempReq);
			}
			else
			{	bspInitFlag = false;	//	Need to notify PowerPlug
				bspPointTemp = (Vector **)NewHandle(pointTempReq);
			}
		}

		clippingCacheMem = header.uniqueEdgeCount*(long)sizeof(short);
		edgeCacheMem = header.uniqueEdgeCount*sizeof(EdgeCacheRecord);
		if(edgeCacheMem < 2*sizeof(Matrix))
		{	edgeCacheMem = 2*sizeof(Matrix);
		}

		clippingCacheHandle = (short **)NewHandle(clippingCacheMem);
		edgeCacheHandle = (EdgeCacheRecord **) NewHandle(edgeCacheMem);

		if(colorLookupSize < header.colorCount)
		{
			long	colorLookupMem;
			
			colorLookupSize = header.colorCount;
			colorLookupMem = colorLookupSize*(long)sizeof(ColorRecord *);

			if(bspColorLookupTable)
				SetHandleSize((Handle)bspColorLookupTable, colorLookupMem);
			else
			{	bspColorLookupTable = (ColorRecord ***)NewHandle(colorLookupMem);
				bspInitFlag = false;	//	Need to notify PowerPlug
			}
		}
		
		if(!bspIndexStack)
		{	bspIndexStack = (short *)NewPtr(INDEXSTACKDEPTH * sizeof(short));
			bspInitFlag = false;		//	Need to notify PowerPlug
		}

		HUnlock(itsBSPResource);
		
		BuildBoundingVolumes();
	}
	
	Reset();

	if(bspInitFlag == false)
	{	InitBSPPlug		*initPlug;
	
		bspInitFlag = true;
		bspLightingPower = GetPowerRoutine(NULL, PAKITPOWERPLUGID, kBSPLighting);
		bspClipEdgesPower = GetPowerRoutine(NULL, PAKITPOWERPLUGID, kBSPClipping);
		bspDrawPolygons = GetPowerRoutine(NULL, PAKITPOWERPLUGID, kBSPDrawPolygons);
		bspPreProcessPower = GetPowerRoutine(NULL, PAKITPOWERPLUGID, kBSPPreProcess);
		initPlug = GetPowerRoutine(NULL, PAKITPOWERPLUGID, kBSPInitPlug);

		if(initPlug)
		{	BSPGlobalsInfoRecord	globs;
		
			globs.bspIndexStack = bspIndexStack;
			globs.bspPointTemp = bspPointTemp;
			globs.colorLevelOne = COLORLEVELONE;
			globs.colorCacheSize = COLORCACHESIZE;
			globs.bspColorLookupTable = bspColorLookupTable;
			globs.viewParameterObjectOffset = (long)&(((CViewParameters *)0)->xOffset);
			globs.tempLockCountPointer = &tempLockCount; 
			initPlug(&globs);
		}
	}
}

void	CBSPPart::Dispose()
{
	if(itsBSPResource)
	{	BSPResourceHeader	*bp;
	
		bp = (BSPResourceHeader *)*itsBSPResource;
		bp->refCount--;
		
		if(bp->refCount == 0)
		{	ReleaseResource(itsBSPResource);
		}
		
		if(colorReplacements)
		{	DisposeHandle(colorReplacements);
		}
		DisposeHandle((Handle)faceTemp);
		DisposeHandle((Handle)edgeCacheHandle);
		DisposeHandle((Handle)clippingCacheHandle);
	}
	inherited::Dispose();
}

#define	TESTBOUND(b,c)	(otherCorners[0][b] c boundary) && \
						(otherCorners[1][b] c boundary) && \
						(otherCorners[2][b] c boundary) && \
						(otherCorners[3][b] c boundary) && \
						(otherCorners[4][b] c boundary) && \
						(otherCorners[5][b] c boundary) && \
						(otherCorners[6][b] c boundary) && \
						(otherCorners[7][b] c boundary)

Boolean	CBSPPart::HopeNotObscure(
register	CBSPPart *other,
register	Vector	 *otherCorners)
{
	Fixed	boundary;

	if(localViewOrigin[0] < header.minBounds.x)
	{	boundary = header.minBounds.x + tolerance;
		if(TESTBOUND(0,<=))		return false;
	}

	if(localViewOrigin[0] > header.maxBounds.x)
	{	boundary = header.maxBounds.x - tolerance;
		if(TESTBOUND(0,>=)) 	return false;
	}

	if(localViewOrigin[1] < header.minBounds.y)
	{	boundary = header.minBounds.y + tolerance;
		if(TESTBOUND(1,<=)) 	return false;
	}

	if(localViewOrigin[1] > header.maxBounds.y)
	{	boundary = header.maxBounds.y - tolerance;
		if(TESTBOUND(1,>=)) 	return false;
	}

	if(localViewOrigin[2] < header.minBounds.z)
	{	boundary = header.minBounds.z + tolerance;
		if(TESTBOUND(2,<=)) 	return false;
	}

	if(localViewOrigin[2] > header.maxBounds.z)
	{	boundary = header.maxBounds.z - tolerance;	
		if(TESTBOUND(2,>=))		return false;
	}

	return true;
}

Boolean	CBSPPart::HopeDoesObscure(
register	CBSPPart *other,
register	Vector	 *otherCorners)
{
	Fixed	boundary;

	if(localViewOrigin[0] > header.minBounds.x)
	{	boundary = header.minBounds.x + tolerance;
		if(TESTBOUND(0,<=))		return false;
	}

	if(localViewOrigin[0] < header.maxBounds.x)
	{	boundary = header.maxBounds.x - tolerance;
		if(TESTBOUND(0,>=)) 	return false;
	}

	if(localViewOrigin[1] > header.minBounds.y)
	{	boundary = header.minBounds.y + tolerance;
		if(TESTBOUND(1,<=)) 	return false;
	}

	if(localViewOrigin[1] < header.maxBounds.y)
	{	boundary = header.maxBounds.y - tolerance;
		if(TESTBOUND(1,>=)) 	return false;
	}

	if(localViewOrigin[2] > header.minBounds.z)
	{	boundary = header.minBounds.z + tolerance;
		if(TESTBOUND(2,<=)) 	return false;
	}

	if(localViewOrigin[2] < header.maxBounds.z)
	{	boundary = header.maxBounds.z - tolerance;	
		if(TESTBOUND(2,>=))		return false;
	}

	return true;
}
#define	DOESOBSCURE(a)			a->visibilityScore = true;
#define	DOESNOTOBSCURE(a)		a->visibilityScore = false;

#define	DO_PROFILE	0
#if DO_PROFILE
#define	PROFILING_CODE(a)		a

extern	Boolean	newWayFlag;
static	long	caseTable[256];
static	long	totalCases = 0;
#else
#define	PROFILING_CODE(a)		
#endif

Boolean	CBSPPart::Obscures(
	CBSPPart	*other)
{
	Fixed	*v;
	Fixed	delta;
	Fixed	sum;
	Vector	deltaV;
	Vector	center;
	Vector	otherCorners[8];
	Vector	myCorners[8];
	Matrix	combinedTransform;
	short	didTests = 0;
	Fixed	r;

	PROFILING_CODE(short	theCase = 0;)
	
	PROFILING_CODE(totalCases++;)

	VectorMatrix34Product(1, &other->sphereGlobCenter, &center, &invGlobTransform);
	r = other->header.enclosureRadius - tolerance;

	if(		(localViewOrigin[0] < header.minBounds.x && center[0] <= header.minBounds.x - r)
		||	(localViewOrigin[0] > header.maxBounds.x && center[0] >= header.maxBounds.x + r)
		||	(localViewOrigin[1] < header.minBounds.y && center[1] <= header.minBounds.y - r)
		||	(localViewOrigin[1] > header.maxBounds.y && center[1] >= header.maxBounds.y + r)
		||	(localViewOrigin[2] < header.minBounds.z && center[2] <= header.minBounds.z - r)
		||	(localViewOrigin[2] > header.maxBounds.z && center[2] >= header.maxBounds.z + r))
	{	PROFILING_CODE(caseTable[1]++;)
		return false;
	}
	
	VectorMatrix34Product(1, &sphereGlobCenter, &center, &other->invGlobTransform);
	r = header.enclosureRadius - other->tolerance;

	if(		(other->localViewOrigin[0] > other->header.minBounds.x && center[0] <= other->header.minBounds.x - r)
		||	(other->localViewOrigin[0] < other->header.maxBounds.x && center[0] >= other->header.maxBounds.x + r)
		||	(other->localViewOrigin[1] > other->header.minBounds.y && center[1] <= other->header.minBounds.y - r)
		||	(other->localViewOrigin[1] < other->header.maxBounds.y && center[1] >= other->header.maxBounds.y + r)
		||	(other->localViewOrigin[2] > other->header.minBounds.z && center[2] <= other->header.minBounds.z - r)
		||	(other->localViewOrigin[2] < other->header.maxBounds.z && center[2] >= other->header.maxBounds.z + r))
	{	PROFILING_CODE(caseTable[2]++;)
		return false;
	}

	if(	(localViewOrigin[0] < header.minBounds.x) ||
		(localViewOrigin[0] > header.maxBounds.x) ||
		(localViewOrigin[1] < header.minBounds.y) ||
		(localViewOrigin[1] > header.maxBounds.y) ||
		(localViewOrigin[2] < header.minBounds.z) ||
		(localViewOrigin[2] > header.maxBounds.z))
	{	didTests = 1;
	
		CombineTransforms(		&itsTransform,
								&combinedTransform,
								&other->invGlobTransform);
	
		TransformBoundingBox(&combinedTransform, 
								&header.minBounds.x,
								&header.maxBounds.x,
								myCorners);
		if(!other->HopeDoesObscure(this, myCorners))
		{	PROFILING_CODE(caseTable[3]++;)
			return false;
		}
	}

	if(	(other->localViewOrigin[0] > other->header.minBounds.x) ||
		(other->localViewOrigin[0] < other->header.maxBounds.x) ||
		(other->localViewOrigin[1] > other->header.minBounds.y) ||
		(other->localViewOrigin[1] < other->header.maxBounds.y) ||
		(other->localViewOrigin[2] > other->header.minBounds.z) ||
		(other->localViewOrigin[2] < other->header.maxBounds.z))
	{
		didTests |= 2;
		CombineTransforms(		&other->itsTransform,
								&combinedTransform,
								&invGlobTransform);
	
		TransformBoundingBox(&combinedTransform, 
							&other->header.minBounds.x,
							&other->header.maxBounds.x,
							otherCorners);
	
		if(!HopeNotObscure(other, otherCorners))
		{	PROFILING_CODE(caseTable[6]++;)
			return false;
		}
	}

#ifdef NOT_WORTH_IT

	if(visibilityScore)
	{	if(	((didTests & 1) && HopeDoesObscure(other, otherCorners)) &&
			((didTests & 2) && other->HopeNotObscure(this, myCorners)))
		{	visibilityScore = maxZ < other->maxZ;
		}
		else
			theCase = 4;
	}

	if(visibilityScore)
	{	v = other->sphereCenter;
	
		deltaV[0] = sphereCenter[0]-v[0];
		deltaV[1] = sphereCenter[1]-v[1];
		deltaV[2] = sphereCenter[2]-v[2];
	
		delta = FDistanceEstimate(deltaV[0], deltaV[1], deltaV[2]);
		sum = header.enclosureRadius+other->header.enclosureRadius;
		
		if(delta >= sum)
		{	delta = VectorLength(3, deltaV);
		}
	
		if(delta >= sum)
		{	Fixed	dist;
		
			dist =	-	FMulDivNZ(deltaV[0], v[0],delta);
			dist -=		FMulDivNZ(deltaV[1], v[1],delta);
			dist -=		FMulDivNZ(deltaV[2], v[2],delta);
			
			if(dist < other->header.enclosureRadius)
			{	theCase = 5;
				visibilityScore = false;//return false;
			}
		}
	}
#endif

	PROFILING_CODE(caseTable[theCase]++;)

#if 1
	if((didTests == 3)
		&& HopeDoesObscure(other, otherCorners)
		&& other->HopeNotObscure(this, myCorners))
	{
#if DO_PROFILE
		if(maxZ < other->maxZ) caseTable[7]++;
		else					caseTable[8]++;
#endif
	
	//	return maxZ < other->maxZ;
		return minZ+maxZ < other->minZ+other->maxZ;
	}
	else
#endif
	{	PROFILING_CODE(caseTable[9]++;)
		return true;
	}
}

void	CBSPPart::FlipNormals()
{
	short				oldRefCount;
	BSPResourceHeader	*bspResPtr;
	Vector				*v;
	short				i;
	PolyRecord			*p;
	
	bspResPtr = (BSPResourceHeader *)*itsBSPResource;
	oldRefCount = bspResPtr->refCount;
	bspResPtr->refCount = 0;

	v = (Vector *)(header.vectorOffset + *itsBSPResource);
	for(i=0;i<header.vectorCount;i++)
	{	(*v)[0] = -(*v)[0];
		(*v)[1] = -(*v)[1];
		(*v)[2] = -(*v)[2];
		v++;
	}

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);
	for(i=0;i<header.polyCount;i++)
	{	short	temp;
	
		temp = p->frontPoly;
		p->frontPoly = p->backPoly;
		p->backPoly = temp;
		p++;
	}

	ChangedResource(itsBSPResource);
	WriteResource(itsBSPResource);
	bspResPtr->refCount = oldRefCount;
}

void	CBSPPart::WriteToDisk(void)
{
	short				oldRefCount;
	BSPResourceHeader	*bspResPtr;

	bspResPtr = (BSPResourceHeader *)*itsBSPResource;
	oldRefCount = bspResPtr->refCount;
	bspResPtr->refCount = 0;

	ChangedResource(itsBSPResource);
	WriteResource(itsBSPResource);
	bspResPtr->refCount = oldRefCount;
}

void	CBSPPart::UpdateNormalRecords()
{
	NormalRecord		*norms;
	PolyRecord			*p;
	short				i;

	norms = (NormalRecord *)(header.normalOffset + *itsBSPResource);
	for(i=0;i<header.normalCount;i++)
	{	norms[i].visibilityFlags = 0;
	}

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);

	for(i=0;i<header.polyCount;i++)
	{	norms[p->normalIndex].visibilityFlags |= p->visibility;
		p++;
	}
}

void	CBSPPart::SetFaceVisibility(
	short	visibilityFlags)
{
	PolyRecord			*p;
	short				i;

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);
	for(i=0;i<header.polyCount;i++)
	{	p->visibility = visibilityFlags;
		p++;
	}
	
	UpdateNormalRecords();
}

void	CBSPPart::TempDrawAllFaces()
{
	PolyRecord			*p;
	short				i;

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);
	for(i=0;i<header.polyCount;i++)
	{	p->visibility = (p->visibility << 2) + bothVisible;
		p++;
	}
	
	UpdateNormalRecords();
}

void	CBSPPart::RestoreFaceDrawing()
{
	PolyRecord			*p;
	short				i;

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);
	for(i=0;i<header.polyCount;i++)
	{	p->visibility >>= 2;
		p++;
	}
	
	UpdateNormalRecords();
}

void	CBSPPart::AdjustVisibilityFlags(
		CViewParameters	*vp)
{
	short			*visData;
	short			rowCount;
	short			xBound;
	short			index;
	PolyRecord		*polys;
	FaceVisibility	*faces;

	PolyWorld	*polyWorld;
	
	polyWorld = GetPolyWorld();

	TempDrawAllFaces();
	vp->DoLighting();

	if(PrepareForRenderOne(vp) && PrepareForRenderTwo())
	{	DrawNumberedPolygons();
		RestoreFaceDrawing();
		
		visData = VirtualScanConvert(-1);

		xBound = polyWorld->bounds.right;
		rowCount = polyWorld->bounds.bottom - polyWorld->bounds.top;

		polys = (PolyRecord *)(header.polyOffset + *itsBSPResource);
		faces = *faceTemp;
		
		while(rowCount--)
		{	do
			{	index = *visData++;
				if(index >= 0)
				{	polys[index].visibility |= faces[polys[index].normalIndex].visibility;
				}
			} while(*visData++ < xBound);
		}
		UpdateNormalRecords();
		PostRender();
	}
}

void	CBSPPart::ToggleFace(
		CViewParameters	*vp,
		Point			where)
{
	short			*visData;
	short			rowCount;
	short			xBound;
	short			index;
	PolyRecord		*polys;
	FaceVisibility	*faces;

	PolyWorld	*polyWorld;
	
	polyWorld = GetPolyWorld();

	xBound = polyWorld->bounds.right;
	rowCount = polyWorld->bounds.bottom - polyWorld->bounds.top;

	if(rowCount > where.v && xBound > where.h)
	{
		TempDrawAllFaces();
		vp->DoLighting();
	
		if(PrepareForRenderOne(vp) && PrepareForRenderTwo())
		{	DrawNumberedPolygons();
			RestoreFaceDrawing();
			
			visData = VirtualScanConvert(-1);
	
	
			polys = (PolyRecord *)(header.polyOffset + *itsBSPResource);
			faces = *faceTemp;
			
			rowCount = where.v;
			while(rowCount--)
			{	do
				{	index = *visData++;
				} while(*visData++ < xBound);
			}
			
			do
			{	index = *visData++;
			} while(*visData++ < where.h);
			
			polys[index].visibility ^= faces[polys[index].normalIndex].visibility;
			UpdateNormalRecords();
	
			PostRender();
		}
	}
}


void	CBSPPart::GetStatistics(
	long	*stats)
{
	PolyRecord			*p;
	short				i;

	stats[0] = 0;
	stats[1] = 0;
	stats[2] = 0;
	stats[3] = 0;

	p = (PolyRecord *)(header.polyOffset + *itsBSPResource);
	for(i=0;i<header.polyCount;i++)
	{	stats[p->visibility & 3] ++;
		p++;
	}

}

#define WRITENUM(a)	{ NumToString(a, temps); DrawString(temps); DrawChar(','); DrawChar(' '); }
void	CBSPPart::DoStatistics()
{
	long				stat[4];
	Str32				temps;
	GrafPtr				saved;
	GrafPort			deskPort;
	
	GetStatistics(stat);
	
	GetPort(&saved);
	OpenPort(&deskPort);
	SetPort(&deskPort);
	TextMode(srcCopy);
	MoveTo(8,15);
	WRITENUM(stat[0])
	WRITENUM(stat[1])
	WRITENUM(stat[2])	
	WRITENUM(stat[3])
	WRITENUM(stat[0]+stat[1]+stat[2]+stat[3])
	WRITENUM(stat[1]+stat[2]+stat[3]*2)
	DrawChar(' ');
	ClosePort(&deskPort);	
	SetPort(saved);
}
#endif