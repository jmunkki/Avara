/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: BSPPlug.c
    Created: Thursday, May 4, 1995, 13:08
    Modified: Thursday, November 23, 1995, 15:09
/*/

#ifdef powerc
#define	POWERPLUG_CLASS
#include "BSPPlug.h"
#include "CViewParameters.h"

extern	PolyWorld	*thePolyWorld;

static	short				*bspIndexStack = 0;
extern	Vector				**bspPointTemp;
static	short				colorLevelOne;
static	short				colorCacheSize;
extern	ColorRecord			***bspColorLookupTable;
		long				*tempLockCountP = 0;
static	long				viewParameterObjectOffset;

#define	MAX_PLUG_LIGHTS	16

typedef	float	lightCalcType;
typedef	float	ClipCalcType;
typedef	float	FloatVect[4];


static	lightCalcType	divideByOne = 1/65536.0;
static	lightCalcType	divideByOneTwice = 1/(65536.0*65536.0);
static	lightCalcType	multiplyByOne = 65536.0;


/*
**	DrawPolygons used to be a recursive binary space partition tree walk.
**	I removed the recursion and use a stack instead. A negative value (i)
**	on top of the stack means that polygon -i is next in the tree and
**	should push it's "commands" in the stack. A positive value means
**	that you draw a polygon and drop the index from the stack.
*/
void	DrawPolygonsPlugC(
	DrawPolygonsParam	*p)
{
	FaceVisibility		*faceTable;
	EdgeIndex			*edgeTable;
	short				*clippingCacheTable;
	PolyRecord			*thePoly;
	short				*stackP;
	short				*stackTop;
	short				i;
	short				polyVisibility;
	EdgeIndex			*e;
	PolyEdgePtr			*quickEdge;
	EdgeCacheRecord		*edgeCacheTable;
	PolyRecord			*polyTable;

	thePolyWorld = p->polys;
	faceTable = *(p->faceTemp);
	clippingCacheTable = *(p->clippingCacheHandle);
	quickEdge = (PolyEdgePtr *)*(p->bspPointTemp);

	edgeCacheTable = *(p->edgeCacheHandle);
	edgeTable = p->edgeTable;

	stackTop = INDEXSTACKDEPTH + p->bspIndexStack;
	stackP = stackTop;
	
	polyTable = p->polyTable;
	thePoly = polyTable;

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

			thePolyWorld->currentColor = faceTable[thePoly->normalIndex].colorIndex;
			thePolyWorld->polyCount++;

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
						if(previous) ReplicatePolySegmentC(previous);
					}
					else
					{	clippingCacheTable[edgeInd] |= edgeExitsFlag;
						quickEdge[edgeInd] =
							AddFPolySegmentC(screenEdge->ax.f, screenEdge->ay.f,
										screenEdge->bx.f, screenEdge->by.f);
					}
				}
			}

			p = frontList;
			for(j=1;j<frontCount;j+=2)
			{	AddFPolySegmentC(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
				p+=2;
			}
			p = backList;
			for(j=1;j<backCount;j+=2)
			{	AddFPolySegmentC(p[0].x.f, p[0].y.f, p[1].x.f, p[1].y.f);
				p+=2;
			}
		}
	}
}

#ifdef powerc
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
#endif

void	CBSPPart::DoLightingF()
{
	lightCalcType	fastLights[MAX_PLUG_LIGHTS][3];
	lightCalcType	*lv;
	lightCalcType	ambient;
	CViewParameters	*vp = (CViewParameters *)
			(((Ptr)currentView) +
			viewParameterObjectOffset -
			(long)&(((CViewParameters *)0)->xOffset));

	long			lightCount;
	Vector			*pointArray;
	Vector			*vectorArray;
	NormalRecord	*norms;
	ColorRecord		**colorPointers;
	FaceVisibility	*faces;
	lightCalcType	colorLevelAdjust;
	short			colorCacheS;
	Fixed			*objLightsP;

	long			i;
	long			j;

	if(ignoreDirectionalLights)
	{	lightCount = 0;
	}
	else
	{	lightCount = vp->lightSourceCount;
	}

	if(privateAmbient < 0)
	{	ambient = multiplyByOne * (vp->ambientLight + extraAmbient);
	}
	else
	{	ambient =  multiplyByOne * (privateAmbient + extraAmbient);
	}

	vectorArray = (Vector *)(header.vectorOffset + *itsBSPResource);
	pointArray = (Vector *)(header.pointOffset + *itsBSPResource);
	norms = (NormalRecord *)(header.normalOffset + *itsBSPResource);

	colorPointers = *bspColorLookupTable;
	faces = *faceTemp;
	
	colorLevelAdjust  = colorLevelOne * divideByOneTwice;
	colorCacheS = colorCacheSize;

	objLightsP = &objLights[0][0];

	lv = &fastLights[0][0];
	for(i=0;i<lightCount;i++)
	{	*lv++ = *objLightsP++;
		*lv++ = *objLightsP++;
		*lv++ = *objLightsP++;
		objLightsP++;
	}

	i = header.normalCount;
	while(--i >= 0)
	{				lightCalcType	light;
					lightCalcType	dot;
					lightCalcType	x,y,z;
					Fixed			*surfNorm;
					Vector			*surfPoint;
		
		//	Access surface normal and one point on surface.
		surfNorm = vectorArray[norms->normalIndex];
		surfPoint = pointArray + norms->basePointIndex;

		//	Determine if surface faces the user.
		//	This information is used for backface culling and
		//	walking the BSP tree in the right order.
		
		x = surfNorm[0];
		y = surfNorm[1];
		z = surfNorm[2];
		
		dot = 	x * ((*surfPoint)[0] - localViewOrigin[0]);
		dot +=	y * ((*surfPoint)[1] - localViewOrigin[1]);
		dot +=	z * ((*surfPoint)[2] - localViewOrigin[2]);
		
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
			lv = &fastLights[0][0];
			
			j = lightCount;
			while(--j >= 0)
			{	
				dot =	lv[0] * x
					+	lv[1] * y
					+	lv[2] * z;

				if(dot > 0) light += dot;

				lv += 3;	
			}
			
			j = 0.5 + light * colorLevelAdjust;

			if(j < 0)						j = 0;
			else if(j >= colorCacheS)		j = colorCacheS-1;
			
			faces->colorIndex = colorPointers[norms->colorIndex]->colorCache[j];
		}
		norms++;
		faces++;
	}
}
void	CBSPPart::ClipEdgesF()
{
	CViewParameters	*vp;
	long			edgeCount;
	ClipCalcType	fYon;
	ClipCalcType	fHither;
	
	ClipCalcType	fMinZ;
	ClipCalcType	fMaxZ;
	Fixed			rangeMin;
	Fixed			rangeMax = 0x78000000;
	
	EdgeCacheRecord		*clippedLine;
	short				i;
	short				*stat;
	EdgeRecord			*uniEdge;

	ClipCalcType		xOffCopy;
	ClipCalcType		yOffCopy;
	ClipCalcType		negScaleCopy;

	vp = (CViewParameters *)
			(((Ptr)currentView) +
			viewParameterObjectOffset -
			(long)&(((CViewParameters *)0)->xOffset));

	rangeMin = -rangeMax;

	xOffCopy = vp->xOffset;
	yOffCopy = vp->yOffset;
	fYon = yon;
	fHither = hither;
	negScaleCopy = - vp->screenScale;
	
	fMinZ = fYon;
	fMaxZ = fHither;
	minX = 0x7FffFFff;
	maxX = 0x80000000;
	minY = minX;
	maxY = maxX;

	stat = clippingCacheTable;
	clippedLine = edgeCacheTable;
	uniEdge = uniqueEdgeTable;

	edgeCount = header.uniqueEdgeCount;
	for(i=0;i<edgeCount;i++)
	{	if(*stat)
		{	FloatVect		*va, *vb;
			ClipCalcType	vfa[3];
			ClipCalcType	vfb[3];
			ClipCalcType	az, bz;

			va = (FloatVect *)&pointTable[uniEdge->a];
			vb = (FloatVect *)&pointTable[uniEdge->b];
			
			//	Sort endpoints:
			if((*va)[2] > (*vb)[2])
			{	FloatVect	*vt;
				vt = va;
				va = vb;
				vb = vt;
			}
			
			vfa[0] = (*va)[0];
			vfa[1] = (*va)[1];
			vfa[2] = az = (*va)[2];

			vfb[0] = (*vb)[0];
			vfb[1] = (*vb)[1];
			vfb[2] = bz = (*vb)[2];

			if(az >= fYon)
					*stat = noLineClip;	//	Line is too far to be visible
			else
			if(bz <= fHither)
					*stat = noLineClip;	//	Line is too close or behind
			else
			{	ClipCalcType	clipDist;
				
				*stat = lineVisibleClip;
				
				if(az <= fMinZ)
				{
					clipDist = az - fHither;
					if(clipDist < 0)
					{	if(clipDist)
						{	clipDist /= az - bz;
							vfa[0] = vfa[0] + (vfb[0] - vfa[0]) * clipDist;
							vfa[1] = vfa[1] + (vfb[1] - vfa[1]) * clipDist;
							az = fHither;
						}
						*stat |= touchFrontClip;
					}
					
					fMinZ = az;
				}
				
				if(bz >= fMaxZ)
				{	
					clipDist = bz - fYon;
					if(clipDist > 0)
					{	if(clipDist)
						{	clipDist /= bz - az;
							vfb[0] = vfb[0] + (vfa[0] - vfb[0]) * clipDist;
							vfb[1] = vfb[1] + (vfa[1] - vfb[1]) * clipDist;
							bz = fYon;
						}
						*stat |= touchBackClip;
					}
					
					fMaxZ = bz;
				}
				
				{
					Fixed	a,b;
					
					az = negScaleCopy / az;
					a = vfa[0] * az + xOffCopy;					
					bz = negScaleCopy / bz;
					b = vfb[0] * bz + xOffCopy;
					
					if(a < b)
					{	if(a < minX)
						{	if(a < rangeMin)
							{	a = rangeMin;
								if(b < rangeMin)
									b = rangeMin;
							}
							minX = a;
						}
						if(b > maxX)
						{	if(b > rangeMax)
							{	b = rangeMax;
								if(a > rangeMax)
									a = rangeMax;
							}
							maxX = b;
						}
					}
					else
					{	if(b < minX)
						{	if(b < rangeMin)
							{	b = rangeMin;
								if(a < rangeMin)
									a = rangeMin;
							}
							minX = b;
						}
						if(a > maxX)
						{	if(a > rangeMax)
							{	a = rangeMax;
								if(b > rangeMax)
									b = rangeMax;
							}
							maxX = a;
						}
					}
					
					clippedLine->ax.f = a;
					clippedLine->bx.f = b;
	
					a = vfa[1] * az + yOffCopy;
					b = vfb[1] * bz + yOffCopy;

					if(a < b)
					{	if(a < minY)
						{	if(a < rangeMin)
							{	a = rangeMin;
								if(b < rangeMin)
									b = rangeMin;
							}
							minY = a;
						}
						if(b > maxY)
						{	if(b > rangeMax)
							{	b = rangeMax;
								if(a > rangeMax)
									a = rangeMax;
							}
							maxY = b;
						}
					}
					else
					{	if(b < minY)
						{	if(b < rangeMin)
							{	b = rangeMin;
								if(a < rangeMin)
									a = rangeMin;
							}
							minY = b;
						}
						if(a > maxY)
						{	if(a > rangeMax)
							{	a = rangeMax;
								if(b > rangeMax)
									b = rangeMax;
							}
							maxY = a;
						}
					}

					clippedLine->ay.f = a;
					clippedLine->by.f = b;
				}
			}
		}
		stat++;
		clippedLine++;
		uniEdge++;
	}

	minZ = fMinZ;
	maxZ = fMaxZ;
}

void	DrawPolygonsListPlugC(
	DrawPolygonsParam	*p)
{
	CBSPPart	*theObj;
	CBSPPart	**objList;
	long		i;

	objList = p->visibleObjectArray;
	i = p->visibleObjectCount;

	while(i--)
	{	theObj = (CBSPPart *)(((Ptr)(*objList))-2);
		p->faceTemp = theObj->faceTemp;
		p->polyTable = theObj->polyTable;
		p->clippingCacheHandle = theObj->clippingCacheHandle;
		p->edgeCacheHandle = theObj->edgeCacheHandle;
		p->edgeTable = theObj->edgeTable;
		p->bspPointTemp = bspPointTemp;
		DrawPolygonsPlugC(p);

		objList++;
	}
}

Boolean	CBSPPart::PreProcessPartTwoPlug()
{
	Boolean			onScreen;
	Vector			*pointArray;
	CViewParameters	*vp = (CViewParameters *)
							(((Ptr)currentView) +
							viewParameterObjectOffset -
							(long)&(((CViewParameters *)0)->xOffset));

	GenerateColorLookupTable();
	DoLightingF();
	MarkNode();
	MarkPoints();

	pointTable = *bspPointTemp;
	pointArray = (Vector *)(header.pointOffset + *itsBSPResource);
	FlaggedVectorMatrix34ProductF(header.pointCount, pointArray, pointTable, (Matrix *)*edgeCacheHandle);
	ClipEdgesF();

	onScreen = (maxX > 0 && maxY > 0 &&
				 minX < vp->fWidth && minY < vp->fHeight);

	if(!onScreen)
		PostRender();

	return onScreen;
}

void	PreProcessListPlugC(
	PreProcessListParam	*p)
{
	CBSPPart	*theObj;
	CBSPPart	**destList;
	CBSPPart	**objList;
	long		i, visibleCount;

	objList = p->visibleObjectArray;
	destList = objList;
	i = p->visibleObjectCount;
	visibleCount = 0;

	while(i--)
	{	theObj = (CBSPPart *)(((Ptr)(*objList))-2);
		if(theObj->PreProcessPartTwoPlug())
		{	*destList++ = *objList++;
			visibleCount++;
		}
		else
		{	objList++;
		}
	}

	p->visibleObjectCount = visibleCount;
}

void	InitBSPPlugC(
	BSPGlobalsInfoRecord	*globs)
{
	bspIndexStack = globs->bspIndexStack;
	bspPointTemp = globs->bspPointTemp;
	colorLevelOne = globs->colorLevelOne;
	colorCacheSize = globs->colorCacheSize;
	bspColorLookupTable = globs->bspColorLookupTable;
	viewParameterObjectOffset = globs->viewParameterObjectOffset;
	tempLockCountP = globs->tempLockCountPointer;
}

void	BSPPreProcessC(
	Handle	*resReference)
{
	CBSPPart	*it;
	Vector		*pointArray;

	it = (CBSPPart *)(((char *)resReference)
			- ((long)&(((CBSPPart *)0)->itsBSPResource)));
	
	it->DoLightingF();
	it->MarkNode();
	it->MarkPoints();

	it->pointTable = *bspPointTemp;
	pointArray = (Vector *)(it->header.pointOffset + *it->itsBSPResource);
	FlaggedVectorMatrix34ProductF(it->header.pointCount, pointArray, it->pointTable, (Matrix *)*it->edgeCacheHandle);

	it->ClipEdgesF();
}
#endif